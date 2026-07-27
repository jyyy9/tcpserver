#pragma once
#include "../protocol.h"
#include "message.h"

namespace proto{
namespace http {
    class HttpRequest;
    class HttpResponse;
    
    const static int KMaxLine = 4096;
    enum State {
        LINE_RECVING, //首行接收中
        HEAD_RECVING, //头部字段接收中
        BODY_RECVING, //正文接收中
        RECV_COMPLATED, //接收完成
    };
    class HttpController : public Controller{
        public:
            HttpController();
            ~HttpController();
            State state();
            void setState(State state);
            bool recvComplete() const override;
            void recvRequest(net::Buffer* buffer) override;
            void recvResponse(net::Buffer* buffer) override;
            void sendResponse(net::TcpConnectionPtr) override;
            // 客户端一套操作
            void sendRequest(net::TcpConnectionPtr) override;
            void reset() override;
            std::shared_ptr<HttpRequest> request() { return _request; }
            std::shared_ptr<HttpResponse> response() { return _response; }
            static void sendRequest(net::TcpConnectionPtr, HttpRequest*);
        private:
            static std::string serializeResponseLine(const HttpResponse& response);
            static std::string serializeRequestLine(const HttpRequest& request);
            static std::string serializeHead(const std::unordered_map<std::string, std::string>& headers);
            void recvRequestLine(net::Buffer* buffer) ;
            void recvRequestHead(net::Buffer* buffer) ;
            void recvRequestBody(net::Buffer* buffer) ;

            void recvResponseLine(net::Buffer* buf) ;
            void recvResponseHead(net::Buffer* buf) ;
            void recvResponseBody(net::Buffer* buf) ;
        private:
            State _state;
            // 用于接收收到的请求或响应数据，并将解析后的数据放入到对象中
            std::shared_ptr<HttpRequest> _request;
            std::shared_ptr<HttpResponse> _response;
    };
    using HttpControllerPtr = std::shared_ptr<HttpController>;
}
}