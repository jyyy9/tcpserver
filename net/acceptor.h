#pragma once
#include "sockets.h"
#include "eventloop.h"
#include "channel.h"

namespace net {
    // 监听套接字的管理类
    //  获取一个服务器监听地址，创建套接字，对该套接字绑定地址，开始监听，获取新连接，对新连接进行处理
    using NewConnectCallback = std::function<void(int, InetAddress addr)>;
    class Acceptor {
        public:
            // 初始化成员，为channel设置读事件回调函数
            Acceptor(EventLoop* loop, const InetAddress& addr);
            // 接触socketfd的时间监控，并关闭idlefd
            ~Acceptor();
            // 成员操作接口
            void setNewConnnetCallback(NewConnectCallback cb);
            // 开始套接字监听，启动_channel的读事件监控
            void listen();
        private:
            // 获取新连接，调用回调函数对新连接进行处理
            void handleRead(Timestamp recvTime);
        private:
            EventLoop* _loop; //当前监听所属的事件循环
            int _sockfd; // 监听套接字描述符
            Socket _socket;
            Channel _channel;
            int _idlefd; // 空闲占位描述符
            NewConnectCallback _newConnCallback; //新连接处理回调函数
    };
}