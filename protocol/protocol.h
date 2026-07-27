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
            virtual bool recvComplete() const = 0; //判断请求接收是否完整
            virtual void recvRequest(net::Buffer*) = 0;//接收请求
            virtual void sendResponse(net::TcpConnectionPtr) = 0;//发送响应
            // 客户端一套操作
            virtual void recvResponse(net::Buffer*) = 0;//接收响应
            virtual void sendRequest(net::TcpConnectionPtr) = 0;//发送请求

            virtual void reset() = 0; //重置上下文
        protected:
            bool _ok; // 是否处理成功
            std::string _error; // 错误信息
    };
    using ControllerPtr = std::shared_ptr<Controller>;

    class Protocol {
        public:
            // 构造函数
            Protocol() = default;
            // 析构函数
            virtual ~Protocol() = default;
            // 设置上下文
            virtual void setController(net::TcpConnectionPtr conn) = 0;
            virtual ControllerPtr getController(net::TcpConnectionPtr conn) = 0;
            virtual void dispatchRequest(ControllerPtr) = 0;
            virtual void dispatchResponse(ControllerPtr) = 0;
            // 专门给客户端提供的：发送数据的时候使用的连接对象
            virtual void setConnection(net::TcpConnectionPtr) = 0;
            // 请求处理
            void handleRequest(net::TcpConnectionPtr conn, 
                net::Buffer* buf, net::Timestamp rtime) {
                // 请求处理
                // 1. 从连接中取出上下文对象
                ControllerPtr cntl = getController(conn);
                assert(cntl.get() != nullptr);
                while(1) {
                    // 2. 使用上下文对象接收请求数据，进行处理
                    cntl->recvRequest(buf);
                    // 3. 若请求接收并处理出错，则直接进行错误响应
                    if (cntl->isOk() == false) {
                        cntl->sendResponse(conn);
                        cntl->reset();
                        continue;
                    }
                    if (cntl->recvComplete() == false){
                        //请求不完整，则返回，等新数据到来后再次进行处理
                        break;
                    }
                    // 4. 判断请求接收是否完整，
                    // 若完整，则进行分发处理
                    dispatchRequest(cntl);
                    cntl->sendResponse(conn);
                    // 5. 分发处理完毕，开始发送数据，若发送响应完整，则重置上下文数据
                    cntl->reset();
                }
            }
            // 响应处理
            void handleResponse(net::TcpConnectionPtr conn, 
                net::Buffer* buf, net::Timestamp rtime){
                ControllerPtr cntl = getController(conn);
                assert(cntl.get() != nullptr);
                while(1) {
                    // 2. 使用上下文对象接收请求数据，进行处理
                    cntl->recvResponse(buf);
                    // 3. 若请求接收并处理出错，则直接进行错误响应
                    if (cntl->isOk() == false) {
                        dispatchResponse(cntl);
                        cntl->reset();
                        continue;
                    }
                    if (cntl->recvComplete() == false){
                        //响应不完整，则返回，等新数据到来后再次进行处理
                        break;
                    }
                    // 4. 判断请求接收是否完整，
                    // 若完整，则进行分发处理
                    dispatchResponse(cntl);
                    // 5. 分发处理完毕，开始发送数据，若发送响应完整，则重置上下文数据
                    cntl->reset();
                }
            }
        };

    

    using ProtocolPtr = std::shared_ptr<Protocol>;
}