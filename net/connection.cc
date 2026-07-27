#include "connection.h"
#include "channel.h"
#include "sockets.h"
#include "eventloop.h"
#include "log.h"
#include <cstring>
#include <errno.h>

namespace net {
    // 成员初始化，为通信套接字设置套接字选项，需要为channel设置事件处理回调函数
    TcpConnection::TcpConnection(EventLoop* loop, int64_t id, int fd)
        : _id(id)
        , _state(kConnecting)
        , _loop(loop)
        , _sockfd(fd)
        , _channel(new Channel(fd, loop))
        , _socket(new Socket(fd)) {
        _socket->setKeepAlive(true);
        _channel->setReadCallback(std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
        _channel->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
        _channel->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
        _channel->setErrorCallback(std::bind(&TcpConnection::handleError, this));
         // LOG_DEBUG("CONSTRUCT CONNECTION: %p - %d", this, _channel->fd());
    }
    // 没有特别需要额外释放的资源，
    TcpConnection::~TcpConnection(){
        assert(_state == kDisconnected);
        // LOG_DEBUG("DESSTRUCT CONNECTION: %p", this);
    }
    //成员操作接口
    //连接初始化完成时，框架内部调用的函数-启动读事件监控，调用_connectionCallback，设置状态
    // 这个接口是在tcpserver中，有新连接到来，为新连接构造connection对象，设置回调函数，完毕后调用的接口
    void TcpConnection::connectEstablished(){
        _loop->assertInLoopThread();
        assert(_state == kConnecting);
        setState(kConnected); //当前连接状态设置为连接完成，事件监控中
        // 为channel对象设置外部观察者
        _channel->setTie(shared_from_this());
        // 启动读事件监控
        _channel->enableReading();
        if (_connectionCallback) _connectionCallback(shared_from_this());
    }
    // 连接最终释放时最后所调用的接口：解除监控，移除监控管理，设置状态
    //  连接的关闭场景：
    //      1. 外界调用forceCose主动关闭连接（kConnected -> kDisConnecting
    //      2. channel进行事件处理（连接关闭，又无数据可读），调用handleClose进行关闭(kConnected->kDisConnected)
    //      3. TcpServer释放的时候，会将当前所有的连接都去调用connectDistroyed （kConnected->kDisConnected）
    void TcpConnection::connectDistroyed(){
        // LOG_DEBUG("connectDistroyed");
        _loop->assertInLoopThread();
        if (_state == kConnected) {
             setState(kDisconnected);
            //kConnected状态表示连接没有在任何地方进行关闭, 而是直接在tcpserver析构的时候进行的调用
            _channel->disableAll(); //解除监控
            if (_connectionCallback) _connectionCallback(shared_from_this());
        }
         // LOG_DEBUG("connectDistroyed---------");
        _channel->remove(); //移除监控管理
    }  //连接关闭时，框架内部调用的函数
    //发送数据： 如果当前本身就在事件循环中，则直接发送数据；否则则将实际的发送操作打包成一个任务，压入到loop任务池中
    void TcpConnection::send(const std::string& data){
        if (connected() == false) return;
        return send(data.data(), data.size());
    }
    void TcpConnection::send(const void* data, size_t len){
         // LOG_DEBUG("SEND DAA： %s", (const char*)data);
         if (connected() == false) return;
        if (_loop->isInLoopThread()) {
            // LOG_DEBUG("send InLoopThread");
            sendInLoop(data, len);
        }else {
// LOG_DEBUG("send not InLoopThread");
            std::string buf((char*)data, len);
            void (TcpConnection::*func)(const std::string&) = &TcpConnection::sendInLoop;
            _loop->runInLoop(std::bind(func, this, std::move(buf)));
        }
    }
    //这个发送接口才是真正实现数据发送的接口
    void TcpConnection::sendInLoop(const std::string &data){
        // LOG_DEBUG("sendInLoop(const std::string &data)");
        _loop->assertInLoopThread();
        return sendInLoop(data.data(), data.size());
    }
    void TcpConnection::sendInLoop(const void* data, size_t len){
        // LOG_DEBUG("sendInLoop(const void* data, size_t len)");
        _loop->assertInLoopThread();
        // 1. 如果当前没有监控写事件，且当前conn中的发送缓冲区没有数据，这时候就直接发送数据
        //      1. 直接进行数据发送的时候，发送数据过多，导致socket缓冲区满了
        //      这种情况下， 就有可能data中有数据没有发送完毕，这种情况要将剩余数据放入发送缓冲区中
        // 2. 否则将数据放入发送缓冲区中
        ssize_t nwrote = 0; //保存实际发送的数据长度
        ssize_t remaining = len; //保存剩余要发送的数据长度
        bool errFlag = false;
        // LOG_DEBUG(" %d - %lu", _channel->isWriting(), _outputBuffer.readAbleBytes());
        if (_channel->isWriting() == false && _outputBuffer.readAbleBytes() == 0) {
            // LOG_DEBUG("sendInLoop: %s", (char*)data);
            nwrote = sockets::write(_channel->fd(), data, len);
            if (nwrote < 0) {
                // 发送数据出错了
                nwrote = 0;
                if (errno == EPIPE || errno == ECONNRESET) {
                    errFlag = true;
                    LOG_ERROR("数据发送失败： 链接断开");
                }
            }else {
                remaining = len - nwrote; // 记录剩余没有发送出去的数据长度
            }
        }
        if (remaining > 0 && errFlag == false) {
            //有数据还没有发送完毕，且可以进行重新尝试发送
            _outputBuffer.append((const char*)data + nwrote, remaining);
            if (_channel->isWriting() == false) {
                _channel->enableWriting(); //启动写事件监控
            }
        }
        
    }
    // 关闭连接:只能是打包一个任务，然后压入到任务池中，进行操作
    void TcpConnection::forceClose(){
        // LOG_DEBUG("forceClose"); 
        setState(kDisconnecting);
        // 打包任务
        _loop->queueInLoop(std::bind(&TcpConnection::forceCloseInLoop, this));
    }
    void TcpConnection::forceCloseInLoop(){
        // LOG_DEBUG("forceCloseInLoop"); 
        _loop->assertInLoopThread();
        assert(_state == kDisconnecting);
        handleClose();
    }

    //设置给channel的事件处理回调函数
    void TcpConnection::handleRead(Timestamp recvTime){
        _loop->assertInLoopThread();
        // 1. 将数据读取到接收缓冲区中
        ssize_t ret = _inputBuffer.readFd(_channel->fd());
        if (ret > 0) {
            // 2. 调用_messageCallback对缓冲区中的数据进行业务处理
            if (_messageCallback) _messageCallback(shared_from_this(), &_inputBuffer, recvTime);
        }else if (ret == 0) {
            // LOG_DEBUG("连接断开.....");
            handleClose();
        }else {
            LOG_ERROR("读取数据出错: %s", strerror(errno));
        }
    }
    void TcpConnection::handleWrite(){
        _loop->assertInLoopThread();
        if (_channel->isWriting() == false) {
            LOG_ERROR("触发写事件的时候，发现写事件被关闭了");
            return;
        }
        // 1. 将自己发送缓冲区里边的数据，进行send发送
        if (_outputBuffer.readAbleBytes() > 0) {
            ssize_t ret = sockets::write(_channel->fd(), _outputBuffer.peek(), _outputBuffer.readAbleBytes());
            if (ret >= 0) {
                _outputBuffer.retrive(ret);
            }else {
                if (errno == EPIPE || errno == ECONNRESET) {
                    handleClose();
                    return ;
                }
                LOG_ERROR("写入数据出错");
            }
        }
        // 2. 如果缓冲区中没有数据了，则关闭写事件的监控
        if (_outputBuffer.readAbleBytes() == 0) {
            _channel->disableWriting(); //关闭写事件监控
        }
    }
    void TcpConnection::handleClose(){
        _loop->assertInLoopThread();
        assert(_state == kConnected || _state == kDisconnecting);
        setState(kDisconnected);
        _channel->disableAll();
        auto gardThis = shared_from_this(); //先保存了一份智能指针对象
        if (_connectionCallback) _connectionCallback(shared_from_this());
        if (_closeCallback) _closeCallback(shared_from_this());  //必须最后一个被调用
    }
    void TcpConnection::handleError(){
        LOG_ERROR("事件监控出现了错误！！");
    }
}