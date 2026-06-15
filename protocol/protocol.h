/*
    协议头文件: protocol.h
    用于抽象协议相关的策略类与内部的接口
        1. Protocol：协议策略类
        2. Controller：连接上下文抽象类
        3. Message：消息抽象类，用于定义消息的结构和操作
*/
#pragma once
#include <iostream>
#include <string>
#include <memory>
#include <cassert>
#include "../net/connection.h"

namespace proto {
    class Protocol;
    class Controller;
    class Message;

    class Protocol {
        public:
            // 构造函数
            Protocol() = default;
            // 析构函数
            virtual ~Protocol() = default;
            // 设置上下文
            virtual void setController(net::TcpConnectionPtr conn) = 0;
            // 请求处理
            virtual void handleRequest(net::TcpConnectionPtr conn, 
                net::Buffer* buf, net::Timestamp rtime) {
                // 请求处理
                while(1) {
                    // 1. 从连接中取出上下文对象
                    ControllerPtr cntl = std::any_cast<ControllerPtr>(conn->getContext());
                    assert(cntl.get() != nullptr);
                    // 2. 使用上下文对象接收请求数据，进行处理
                    cntl->recvRequest(buf);
                    // 3. 若请求接收并处理出错，则直接进行错误响应
                    if (cntl->isOk() == false) {
                        cntl->sendResponse(conn);
                        cntl->reset();
                        continue;
                    }
                    // 4. 判断请求接收是否完整，
                    if (cntl->isRecvRequestComplete()) {
                        // 若完整，则进行分发处理
                        dispatch(cntl);
                        cntl->sendResponse(conn);
                        // 5. 分发处理完毕，开始发送数据，若发送响应完整，则重置上下文数据
                        if (cntl->isSendResponseComplete()){
                            cntl->reset();
                        }
                    }else {
                        //请求不完整，则返回，等新数据到来后再次进行处理
                        break;
                    }
                }
            }
            // 响应处理
            virtual void handleResponse(net::TcpConnectionPtr, net::Buffer*, net::Timestamp){

            }
            virtual void dispatch(ControllerPtr) = 0;
        };

    /*
        连接上下文抽象类：
            1. 能够判定当前请求或响应是否处理成功
            2. 能够获取出错后的提示错误信息
            3. 接收请求
            4. 判断请求是否接收完毕
            5. 发送响应
            6. 判断响应是否发送完毕
    */
    class Controller {
        public:
            // 构造函数
            Controller():_ok(true) {}
            // 析构函数
            virtual ~Controller() = default;
            void setError(std::string error) { _error = error; }
            void setOk(bool ok) { _ok = ok; }
            // 判断是否处理成功
            bool isOk() const { return _ok; }
            // 获取错误信息
            std::string getError() const { return _error; }
            // 服务端一套操作
            virtual bool isRecvRequestComplete() const = 0;
            virtual bool isSendResponseComplete() const = 0;
            virtual void recvRequest(net::Buffer*) = 0;
            virtual void sendResponse(net::TcpConnectionPtr) = 0;
            // 客户端一套操作
            virtual bool isRecvResponseComplete() const = 0;
            virtual bool isSendRequestComplete() const = 0;
            virtual void recvResponse(net::Buffer*) = 0;
            virtual void sendRequest(net::TcpConnectionPtr) = 0;

            virtual void reset() = 0;
        protected:
            bool _ok; // 是否处理成功
            std::string _error; // 错误信息
    };

    class Message {
        public:
            // 构造函数
            Message() = default;
            // 析构函数
            virtual ~Message() = default;
            // 抽象操作： 接收数据，发送数据，接收是否完毕，发送是否完毕
            virtual void recvData(Controller* controller, net::Buffer*) = 0;
            virtual void sendData(Controller* controller, net::TcpConnectionPtr) = 0;
            virtual bool isRecvComplete() const = 0;
            virtual bool isSendComplete() const = 0;
    };

    using ProtocolPtr = std::shared_ptr<Protocol>;
    using ControllerPtr = std::shared_ptr<Controller>;
    using MessagePtr = std::shared_ptr<Message>;
}