#pragma once
#include "log.h"
#include "timestamp.h"
#include "buffer.h"
#include <atomic>
#include <memory>
#include <functional>
#include <any>

namespace net
{
    class EventLoop;
    class Channel;
    class Socket;
    class TcpConnection;
    using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
    // 通信套接字的管理类：对套接字进行（可读）事件监控，一旦触发事件，接收数据，对数据进行业务处理。
    //  处理完毕后，进行响应

    // 这个ConnectionCallback这是使用者设置的，技能处理新连接建立事件，也能处理连接关闭事件
    using ConnectionCallback = std::function<void(TcpConnectionPtr)>;
    // 这个CloseCallback不是使用者设置的，而是框架内部的TcpServer设置的，用于清理服务器内部资源
    using CloseCallback = std::function<void(TcpConnectionPtr)>;
    // 这个MessageCallback新数据到来的回调函数类型
    using MessageCallback = std::function<void(TcpConnectionPtr, Buffer *, Timestamp)>;

    class TcpConnection : public std::enable_shared_from_this<TcpConnection>
    {
        enum State
        {
            kDisconnected,
            kConnecting,
            kConnected,
            kDisconnecting
        };
        // kConnecting 构建对象时的初始状态，连接建立中，可能当前正在初始化各项数据与回调
        // kConnected ：等到所有的初始化完毕后的状态（当前已经处于事件监控中）
        // kDisconnecting： 连接关闭中（开始关闭了（调用了关闭连接接口），但是资源还没有正式开始释放）
        // kDisconnected： 连接关闭完成状态（已经正式开始释放资源了）
    public:
        // 成员初始化，为通信套接字设置套接字选项，需要为channel设置事件处理回调函数
        TcpConnection(EventLoop *loop, int64_t id, int fd);
        // 没有特别需要额外释放的资源，
        ~TcpConnection();
        // 成员操作接口

        // 连接初始化完成时，框架内部调用的函数-启动读事件监控，调用_connectionCallback，设置状态
        void connectEstablished();
        // 连接最终释放时最后所调用的接口：解除监控，移除监控管理，设置状态
        void connectDistroyed(); // 连接关闭时，框架内部调用的函数
        // 发送数据： 如果当前本身就在事件循环中，则直接发送数据；否则则将实际的发送操作打包成一个任务，压入到loop任务池中
        void send(const std::string &data);
        void send(const void *data, size_t len);

        // 关闭连接:只能是打包一个任务，然后压入到任务池中，进行操作
        void forceClose();

        int id() { return _id; }
        EventLoop *loop() { return _loop; }
        bool connected() const { return _state == kConnected; }
        bool disconnected() const { return _state == kDisconnected; }
        void setCloseCallback(const CloseCallback &cb)
        {
            _closeCallback = cb;
        }
        void setConnectionCallback(const ConnectionCallback &cb)
        {
            _connectionCallback = cb;
        }
        void setMessageCallback(const MessageCallback &cb)
        {
            _messageCallback = cb;
        }
        void setState(State s) { _state = s; }
        void setContext(const std::any &context) { _context = context; }
        const std::any &getContext() const { return _context; }
        std::any *getMutableContext() { return &_context; }

    private:
        // 这个发送接口才是真正实现数据发送的接口
        void sendInLoop(const std::string &data);
        void sendInLoop(const void *data, size_t len);
        void forceCloseInLoop();
        // 设置给channel的事件处理回调函数
        void handleRead(Timestamp recvTime);
        void handleWrite();
        void handleClose();
        void handleError();

    private:
        int64_t _id; // 当前连接的标识符

        std::atomic<State> _state; // 当前连接状态
        EventLoop *_loop;          // 当前连接所属的事件循环
        int _sockfd;
        std::unique_ptr<Channel> _channel;
        std::unique_ptr<Socket> _socket;
        Buffer _inputBuffer;                    // 接收缓冲区
        Buffer _outputBuffer;                   // 输出缓冲区
        CloseCallback _closeCallback;           // 内部资源清理的回调函数（这个回调函数是tcpserver设置的）
        ConnectionCallback _connectionCallback; // 用户设置的连接建立成功与关闭的回调函数
        MessageCallback _messageCallback;       // 用户设置的新消息业务处理回调函数

        std::any _context; // 上下文对象 , any容器可以存放任意类型的一个对象
    };
}