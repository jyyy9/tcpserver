#pragma once 

#include "log.h"
#include "sockets.h"
#include "eventloop.h"
#include "acceptor.h"
#include "connection.h"

namespace net {
    // 打包所有的功能模块，实现一个服务器类
    class TcpServer {
        public:
            TcpServer(EventLoop* loop, const InetAddress& addr);
            ~TcpServer();
            //启动服务器的接口--开始监听，获取新链接进行处理,启动事件循环池
            void start();
            //设置业务处理回调函数
            void setConnectionCallback(const ConnectionCallback& cb) { 
                _connectionCallback = cb; }
            void setMessageCallback(const MessageCallback& cb) { 
                _messageCallback = cb; }
            // 设置事件循环池数量
            void setThreadNum(int count) {
                _pool.setLoopThreadNum(count);
            }
        private:
            // 设置给acceptor的新链接处理回调函数: 添加链接管理
            //   为描述符构造connection对象，设置回调函数，调用estab
            void onNewConnnection(int fd, InetAddress addr);
            // 链接关闭时的回调函数，设置给connection的closeCallback
            //   1. 移除连接管理 ；  2. 调用connection的connectDestroyed
            void removeConnection(TcpConnectionPtr conn);
            void removeConnectionInLoop(TcpConnectionPtr conn);
        private:
            EventLoop* _baseloop;
            Acceptor _acceptor; //监听套接字管理对象
            std::atomic<int64_t> _next_connid; //递增的链接对象标识ID
            EventLoopThreadPool _pool; //事件循环池
            std::unordered_map<int64_t, TcpConnectionPtr> _connections; //所有的连接对象
            ConnectionCallback _connectionCallback;//用户设置的连接建立成功与关闭的回调函数
            MessageCallback _messageCallback;// 用户设置的新消息业务处理回调函数
    };
}