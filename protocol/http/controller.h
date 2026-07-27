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
            bool recvComplete() const override;
            void recvRequest(net::Buffer* buffer) override {
                switch (_state) {
                    case LINE_RECVING:
                        recvRequestLine(buffer);
                        if(isOk() == false) break;
                    case HEAD_RECVING:
                        recvRequestHead(buffer);
                        if(isOk() == false) break;
                    case BODY_RECVING:
                        recvRequestBody(buffer);
                        if(isOk() == false) break;
                }
            }
            void recvResponse(net::Buffer* buffer) override {
                switch (_state) {
                    case LINE_RECVING:
                        recvResponseLine(buffer);
                        if(isOk() == false) break;
                    case HEAD_RECVING:
                        recvResponseHead(buffer);
                        if(isOk() == false) break;
                    case BODY_RECVING:
                        recvResponseBody(buffer);
                        if(isOk() == false) break;
                }
            }
            void sendResponse(net::TcpConnectionPtr) override;
            // 客户端一套操作
            void sendRequest(net::TcpConnectionPtr) override;
            static void sendRequest(net::TcpConnectionPtr, HttpRequest*);
            void reset() override {
                _state = LINE_RECVING;
                _request.reset(new HttpRequest);
                _response.reset(new HttpResponse);
            }
            std::shared_ptr<HttpRequest> request() { return _request; }
            std::shared_ptr<HttpResponse> response() { return _response; }
        private:
            std::string serializeResponseLine(const HttpResponse& response);
            std::string serializeRequestLine(const HttpRequest& request);
            std::string serializeHead(const std::unordered_map<std::string, std::string>& headers);
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