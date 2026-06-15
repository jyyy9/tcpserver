#include "client.h"
#include "log.h"
#include <cstring>

namespace net {
    TcpClient::TcpClient(EventLoop* loop, const InetAddress& addr)
        : _srvAddr(addr)
        , _loop(loop) {}
    TcpClient::~TcpClient(){
        disconnected();
    }
    //连接服务器，连接成功后，在对应的回调函数中进行条件变量唤醒
    void TcpClient::connect(){
        // 1. 创建套接字
        int sockfd = sockets::createSocket(true); //创建一个阻塞套接字
        // 2. 连接服务器
        
        // 3. 若连接出错，则针对不同的错误情况进行不同的处理
        //  1. 出现了致命错误：直接退出程序
        //  2. 出现了允许重连的错误：retry延迟重新连接服务器
        //  3. 连接成功，则在事件循环中调用newConnection
        int ret = sockets::connect(sockfd, _srvAddr.getAddress());
        int errNo = ret == 0 ? 0 : errno;
        switch(errNo) {
            case 0: //成功的情况
            case EINTR: // 阻塞操作被信号打断
            case EISCONN: //该套接字连接已完成
            case EINPROGRESS: //连接正在进行中
                //调用newConnection
                _loop->runInLoop(std::bind(&TcpClient::newConnection, this, sockfd));
                break;
            case EADDRINUSE: //地址被占用
            case EAGAIN:  //资源暂时不足
            case EALREADY: //该套接字之前的连接未完成
            case ECONNREFUSED: // 连接被拒绝-服务端未启动
            case ENETUNREACH: //网络不可达
            case ETIMEDOUT: //连接超时
                // 进行连接重试
                retry(sockfd);
                break;
            case EACCES: 
            case EPERM:  //权限不足
            case EADDRNOTAVAIL: //地址信息无效
            case EAFNOSUPPORT: // 地址域类型暂不支持
            case EBADF: //坏的文件描述符
            case EFAULT: //地址空间不够用了
            case ENOTSOCK: // 这不是一个套接字描述符
            case EPROTOTYPE: //套接字类型错误，比如使用udp连接tcp服务器
            default:
                LOG_FATAL("连接服务器失败: %s", strerror(errno));
                break;
        }
    } 
    //阻塞获取连接成功后的连接对象:_connection
    TcpConnectionPtr TcpClient::connection(){
        std::unique_lock<std::mutex> lock(_mutex);
        _cond.wait(lock, [this](){ return (bool)_connection; });
        return _connection;
    } 
    //断开连接
    void TcpClient::disconnected(){
        if (_connection) {
            _connection->forceClose();
        }
    } 
    //连接重试接口
    void TcpClient::retry(int fd){
        LOG_DEBUG("重新尝试连接服务器");
        // 1. 关闭fd描述符
        sockets::close(fd);
        // 2. 添加定时任务： delayRetry
        _increRetryTime = _increRetryTime == 0 ? 3 : _increRetryTime * 2;
        if (_increRetryTime > maxRetrytime) {
            _increRetryTime = maxRetrytime;
        }
        _loop->runAfter(_increRetryTime, std::bind(&TcpClient::delayRetry, this));
    } 
    // 延迟定时任务接口
    void TcpClient::delayRetry(){
        connect();
    } 
    // 连接建立时的回调函数：为连接成功的描述符构造connection对象
    // 对于connection对象初始化完毕后不要忘了调用connectEstablished
    void TcpClient::newConnection(int fd){
        std::unique_lock<std::mutex> lock(_mutex);
        _increRetryTime = 0;
        _connection.reset(new TcpConnection(_loop, 0, fd));
        _connection->setConnectionCallback(_connectionCallback);
        _connection->setMessageCallback(_messageCallback);
        _connection->setCloseCallback(std::bind(&TcpClient::removeConnection, this, _connection));
        _connection->connectEstablished();
        _cond.notify_all();
    } 
    // 连接断开时的回调函数：重置_connection成员,调用connectDestroyed
    void TcpClient::removeConnection(TcpConnectionPtr conn){
        LOG_DEBUG("removeConnection");
        conn->connectDistroyed();
        _connection.reset();
        // _loop->runInLoop(std::bind(&TcpClient::removeConnectionInLoop, this, conn));
    }
    void TcpClient::removeConnectionInLoop(TcpConnectionPtr conn) {
    }
}