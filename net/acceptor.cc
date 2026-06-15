#include "acceptor.h"
#include "log.h"

namespace net {
    // 初始化成员，为channel设置读事件回调函数
    Acceptor::Acceptor(EventLoop* loop, const InetAddress& addr)
        : _loop(loop)
        , _sockfd(sockets::createSocket())
        , _socket(_sockfd)
        , _channel(_sockfd, loop)
        , _idlefd(::open("/dev/null", O_CLOEXEC | O_WRONLY)) {
        _socket.bind(addr);
        _socket.setReuseAddr(true);
        _channel.setReadCallback(std::bind(&Acceptor::handleRead, this, std::placeholders::_1));
    }
    // 接触socketfd的事件监控，并关闭idlefd
    Acceptor::~Acceptor(){
        _channel.disableAll();
        _channel.remove();
        ::close(_idlefd);
    }
    // 成员操作接口
    void Acceptor::setNewConnnetCallback(NewConnectCallback cb){
        _newConnCallback = std::move(cb);
    }
    // 开始套接字监听，启动_channel的读事件监控
    void Acceptor::listen(){
        _socket.listen();
        _channel.enableReading();
    }

    // 获取新连接，调用回调函数对新连接进行处理
    void Acceptor::handleRead(Timestamp recvTime){
        InetAddress addr;
        int newfd = _socket.accept(&addr);
        if (newfd >= 0) {
            // 获取成功了
            if (_newConnCallback) _newConnCallback(newfd, addr);
            else {
                LOG_FATAL("监听管理对象未设置新连接处理回调");
            }
        }else {
            // 出错了: 能走过来的都是可以被原谅的错误，为了方式套接字没有取出来
            ::close(_idlefd); //关闭idle描述，是为了腾出一个描述符位子
            struct sockaddr_in addr;
            _idlefd = sockets::accept(_sockfd, &addr); //用腾出来的位子把套接字取出来，不在让监听套接字持续触发事件
            ::close(_idlefd); // 关闭这个取出来的套接字（因为这个描述符是为了占位的，不是为了处理通信连接的）
            _idlefd = ::open("/dev/null", O_CLOEXEC | O_WRONLY); //然后重新打开文件，使用idlefd占位
        }
    }
}