#include "controller.h"
#include "message.h"

namespace proto {
namespace http {
    HttpController::HttpController()
        : _status(200)
        , _request(new HttpRequest)
        , _response(new HttpResponse) {}
    HttpController::~HttpController() {
        _request.reset();
        _response.reset();
    }
    bool HttpController::isRecvRequestComplete() const  { 
        return _request->isRecvComplete();
    }
    bool HttpController::isSendResponseComplete() const  { 
        return _response->isSendComplete();
    }
    void HttpController::recvRequest(net::Buffer* buf)  {
        return _request->recvData(this, buf);
    }
    void HttpController::sendResponse(net::TcpConnectionPtr conn)  {
        if (isOk() == false) {
            _response->setStatus(_status);
        }
        return _response->sendData(this, conn);
    }
    // 客户端一套操作
    bool HttpController::isRecvResponseComplete() const  { 
        return _response->isRecvComplete();
    }
    bool HttpController::isSendRequestComplete() const  { 
        return _request->isSendComplete();
    }
    void HttpController::recvResponse(net::Buffer* buf)  {
        return _response->recvData(this, buf);
    }
    void HttpController::sendRequest(net::TcpConnectionPtr conn)  {
        return _request->sendData(this, conn);
    }

    void HttpController::reset() {
        _status = 200;
        _request.reset(new HttpRequest);
        _response.reset(new HttpResponse);
    }
}
}