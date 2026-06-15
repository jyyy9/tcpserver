#include "server.h"

namespace net {
    // 1. 成员的初始化，2. 为acceptor设置新链接处理回调函数
    TcpServer::TcpServer(EventLoop* loop, const InetAddress& addr)
        : _baseloop(loop)
        , _acceptor(loop, addr)
        , _next_connid(0)
        , _pool(loop) {
        _acceptor.setNewConnnetCallback(std::bind(&TcpServer::onNewConnnection, this,
            std::placeholders::_1, std::placeholders::_2));
    }
    //针对当前所有的连接对象，调用connectDistroyed进行释放
    TcpServer::~TcpServer(){
        for (auto &it : _connections) {
            auto connection = it.second;
            it.second.reset();
            connection->loop()->runInLoop(std::bind(&TcpConnection::connectDistroyed, connection));
        }
    }
    //启动服务器的接口--开始监听，获取新链接进行处理,启动事件循环池
    void TcpServer::start(){
        _pool.start(); 
        _acceptor.listen();
    }
    // 设置给acceptor的新链接处理回调函数: 
    //   为描述符构造connection对象，添加链接对象管理, 设置回调函数，调用connectEstablished
    void TcpServer::onNewConnnection(int fd, InetAddress addr){
        EventLoop* ioLoop = _pool.nextLoop();
        int64_t connid = _next_connid.fetch_add(1);
        TcpConnectionPtr conn(new TcpConnection(ioLoop, connid, fd));
        _connections[connid] = conn;
        conn->setConnectionCallback(_connectionCallback);
        conn->setMessageCallback(_messageCallback);
        conn->setCloseCallback(std::bind(&TcpServer::removeConnection, this, std::placeholders::_1));
        ioLoop->runInLoop(std::bind(TcpConnection::connectEstablished, conn));
    }
    // 链接关闭时的回调函数，设置给connection的closeCallback
    //   1. 移除连接管理 ；  2. 调用connection的connectDestroyed
    void TcpServer::removeConnection(TcpConnectionPtr conn){
        _baseloop->runInLoop(std::bind(&TcpServer::removeConnectionInLoop, this, conn));
    }
    void TcpServer::removeConnectionInLoop(TcpConnectionPtr conn){
        int64_t id = conn->id();
        _connections.erase(id);
        conn->loop()->runInLoop(std::bind(&TcpConnection::connectDistroyed, conn));
    }
}