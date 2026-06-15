#pragma once
#include "eventloop.h"
#include "connection.h"
#include "sockets.h"

namespace net {
    /*
        整合封装客户端模块：
            1. 连接服务器
            2. 向服务器发送请求
            3. 设置回调函数：连接建立/断开的回调，收到响应后的消息处理回调
    */
    class TcpClient {
        public:
            TcpClient(EventLoop* loop, const InetAddress& addr);
            ~TcpClient();
            //连接服务器，连接成功后，在对应的回调函数中进行条件变量唤醒
            void connect(); 
            //阻塞获取连接成功后的连接对象
            TcpConnectionPtr connection(); 
            //断开连接
            void disconnected(); 
            //设置回调函数
            void setConnectionCallback(const ConnectionCallback& cb) { 
                _connectionCallback = cb; }
            void setMessageCallback(const MessageCallback& cb) { 
                _messageCallback = cb; }
        private:
            void retry(int fd); //连接重试接口
            void delayRetry(); // 延迟定时任务接口
            // 连接建立时的回调函数：为连接成功的描述符构造connection对象
            // 对于connection对象初始化完毕后不要忘了调用connectEstablished
            void newConnection(int fd); 
            // 连接断开时的回调函数：重置_connection成员,调用connectDestroyed
            void removeConnection(TcpConnectionPtr conn);
            void removeConnectionInLoop(TcpConnectionPtr conn);
        private:
            const static int maxRetrytime = 30;
            int _increRetryTime = 0;
            InetAddress _srvAddr; //服务端地址信息
            std::mutex _mutex;
            std::condition_variable _cond; 
            EventLoop* _loop; //事件循环对象-监控响应的可读事件
            TcpConnectionPtr _connection; //客户端通信连接对象
            ConnectionCallback _connectionCallback;
            MessageCallback _messageCallback;
    };
}