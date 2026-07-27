#include "controller.h"
#include "message.h"

namespace proto {
namespace http {
    HttpController::HttpController()
        : _state(LINE_RECVING)
        , _request(new HttpRequest)
        , _response(new HttpResponse) {}
    HttpController::~HttpController() {
        _request.reset();
        _response.reset();
    }
    void HttpController::reset() {
        _state = LINE_RECVING;
        _request.reset(new HttpRequest);
        _response.reset(new HttpResponse);
    }
    bool HttpController::recvComplete() const  { 
        return _state == RECV_COMPLATED;
    }
    
    void HttpController::recvRequest(net::Buffer* buffer) {
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
    void HttpController::recvResponse(net::Buffer* buffer)  {
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
    void HttpController::sendResponse(net::TcpConnectionPtr conn)  {
        if (_response->_version.empty()) {
            _response->setVersion(_request->getVersion());
        }
        auto content_length = _response->header("Content-Length");
        if (!content_length) {
            if (_response->_body.empty()) {
                _response->setHeader("Content-Length", std::to_string(0));
            }else {
                _response->setHeader("Content-Length", std::to_string(_response->_body.size()));
            }
        }
        auto connection = _response->header("Connection");
        if (!connection) {
            auto req_connection = _request->header("Connection");
            if (req_connection) {
                _response->setHeader("Connection", *req_connection);
            }else {
                //若是1.0则设置close，否则设置keep-alive
                if (_response->_version == "HTTP/1.0") {
                    _response->setHeader("Connection", "close");
                }else {
                    _response->setHeader("Connection", "keep-alive");
                }
            }
        }

        std::stringstream ss;
        //1. 获取序列化后的响应首行
        ss << serializeResponseLine(*_response.get());
        //2. 获取序列化后的响应头部
        ss << serializeHead(_response->_headers);
        //3. 获取响应正文
        ss << _response->_body;
        //发送响应数据
        // LOG_DEBUG("响应数据: [%s]", ss.str().c_str());
        conn->send(ss.str());
    }
    void HttpController::sendRequest(net::TcpConnectionPtr conn)  {
        return sendRequest(conn, _request.get());
    }
    void HttpController::sendRequest(net::TcpConnectionPtr conn, HttpRequest* request) {
        if (request->_version.empty()) {
            request->setVersion("HTTP/1.1");
        }
        auto content_length = request->header("Content-Length");
        if (!content_length) {
            if (request->_body.empty()) {
                request->setHeader("Content-Length", std::to_string(0));
            }else {
                request->setHeader("Content-Length", std::to_string(request->_body.size()));
            }
        }
        
        auto connection = request->header("Connection");
        if (!connection) {
            //若是1.0则设置close，否则设置keep-alive
            if (request->_version == "HTTP/1.0") {
                request->setHeader("Connection", "close");
            }else {
                request->setHeader("Connection", "keep-alive");
            }
        }
        std::stringstream ss;
        //1. 获取序列化后的请求首行
        ss << serializeRequestLine(*request);
        //2. 获取序列化后的请求头部
        ss << serializeHead(request->_headers);
        //3. 获取响应正文
        ss << request->_body;
        //发送响应数据
        conn->send(ss.str());
    }
    std::string HttpController::serializeResponseLine(const HttpResponse& response) {
        std::stringstream ss;
        ss << response._version << " " << response._status << " ";
        ss << getStatusDesc(response._status) << "\r\n";
        return ss.str();
    }
    std::string HttpController::serializeRequestLine(const HttpRequest& request) {
        std::stringstream ss;
        ss << request._method << " ";
        ss << request._path;
        if (request._query.empty() == true) {
            std::stringstream param_ss;
            param_ss << "?";
            for (auto &param : request._query) {
                param_ss << param.first << "=" << param.second << "&";
            }
            std::string str = param_ss.str();
            if (str.back() == '&') str.pop_back();
            ss << str;
        }
        ss << " ";
        ss << "HTTP/" << request._version;
        ss << "\r\n";
        return ss.str();
    }
    std::string HttpController::serializeHead(const std::unordered_map<std::string, std::string>& headers) {
        std::stringstream ss;
        for (auto &header : headers) {
            ss << header.first << ": " << header.second << "\r\n";
        }
        ss << "\r\n";
        return ss.str();
    }
    State HttpController::state() { return _state; }
    void HttpController::setState(State state) { _state = state; }
    void HttpController::recvRequestLine(net::Buffer* buffer) {
        // GET /path?query=value HTTP/1.1  请求首行的处理
        if (_state != LINE_RECVING) return;
        // LOG_DEBUG("recvLine .....");
        auto line = buffer->getline();
        // 缓冲区中有数据，但是不足一行，但是数据量超过最大行长度
        if(!line && buffer->readAbleBytes() > KMaxLine) {
            setError("line is too long, max line is 4096");
            setOk(false);
            _response->setStatus(414);// 414 URI Too Long
            return;
        }else if(!line && buffer->readAbleBytes() < KMaxLine) {
            //当前首行数据不足
            LOG_DEBUG("数据量不足");
            return;
        }
        // get /path?query=value HTTP/1.1

        std::regex re(R"(^([A-Z]+)\s+(/[^?#]*)(\?([^#\s]*))?\s+(HTTP/\d+\.\d+)$)");
        std::smatch match;
        if (std::regex_match(*line, match, re)) {
            const std::string method = match[1].str();
            for (auto &c : method) {
                _request->_method.push_back(std::toupper(c));
            }

            std::string path = match[2].str();
            bool ret = urlDecode(path, _request->_path, false);
            if(ret == false) {
                setOk(false);
                setError("查询字符串解析失败");
                _response->setStatus(400);
                return;
            }

            std::string params = match[4].str(); // 查询参数：key=val&key2=val2
            _request->_version = match[5].str();
            //针对查询字符串进行解析，设置到_query中, 以&符号进行分割，在以=分割，分割后对key和val进行url解码
            std::vector<std::string> queryParams;
            split(params, "&", queryParams);
            for (auto &query : queryParams) {
                size_t pos = query.find('=');
                if(pos == std::string::npos) {
                    continue;
                }
                std::string key = query.substr(0, pos);
                std::string val = query.substr(pos + 1);
                std::string dkey, dval;
                bool ret = urlDecode(key, dkey, true);
                if(ret == false) {
                    setOk(false);
                    setError("查询字符串解析失败");
                    _response->setStatus(400);
                    return;
                }
                ret = urlDecode(val, dval, true);
                if(ret == false) {
                    setOk(false);
                    setError("查询字符串解析失败");
                    _response->setStatus(400);
                    return;
                }
                _request->_query[dkey] = dval; //避免使用setQuery将解码后的数据重新编码
            }
        }else {
            setError("line is not http request line");
            setOk(false);
            _response->setStatus(400); // 400 Bad Request
            return;
        }
        setState(HEAD_RECVING); // 切换到请求头接收状态
    }
    void HttpController::recvRequestHead(net::Buffer* buffer) {
        if (_state != HEAD_RECVING) return ;
        while(true) {
            //1. 获取一行数据
            auto line = buffer->getline();
            //  1. 获取行失败，但是缓冲区中数据长度过长，则返回错误
            if (!line && buffer->readAbleBytes() >= KMaxLine) {
                setError("line is too long, max line is 4096");
                setOk(false);
                _response->setStatus(414); // 414 URI Too Long
                return;
            }else if (!line && buffer->readAbleBytes() < KMaxLine) {
                LOG_DEBUG("数据量不足: %lu", buffer->readAbleBytes());
                return;
            }
            //  3. 获取了一行数据
            if (line->empty()) {
                //空行代表头部处理结束
                setState(BODY_RECVING);
                return;
            }
            //2. 对这行数据以: 为间隔进行分割，得到key和val
            size_t pos = line->find(": ");
            if (pos == std::string::npos) {
                setError("bad request");
                setOk(false);
                _response->setStatus(400);
                return;
            }
            //3. 将key和val添加到_headers中
            std::string key = line->substr(0, pos);
            std::string val = line->substr(pos + 2);
            _request->_headers[key] = val;
        }
    }
    void HttpController::recvRequestBody(net::Buffer* buffer) {
        if (_state != BODY_RECVING) return ;
        // 从缓冲区中取出数据，放到_body中
        // 1. 通过头部字段中的Content-Length获取正文长度
        size_t totalLen = _request->getContentLength();
        if (totalLen == 0) {
            setState(RECV_COMPLATED);
            return;
        }
        // 2. 若缓冲区中数据长度大于正文长度则只获取正文长度数据到body中
        // 3. 若_body中的数据长度，等于Content-Length，则正文接收完毕，设置状态
        size_t realLen = totalLen - _request->_body.length(); // 剩余需要接收的长度
        realLen = realLen > buffer->readAbleBytes() ? buffer->readAbleBytes() : realLen;
        _request->_body.append(buffer->retriveAsString(realLen));
        if (_request->_body.length() == totalLen) {
            setState(RECV_COMPLATED);
            return;
        }
    }

    void HttpController::recvResponseLine(net::Buffer* buf) {
        if (_state != LINE_RECVING) return ;
        auto line = buf->getline();
        if (!line && buf->readAbleBytes() >= KMaxLine) {
            setOk(false);
            setError("首行大小超过限制");
            return ;
        }else if (!line && buf->readAbleBytes() < KMaxLine) {
            LOG_DEBUG("响应数据量不足");
            return;
        }
        std::vector<std::string> lineArr;
        split(*line, " ", lineArr);
        if (lineArr.size() != 3) {
            setOk(false);
            setError("首行大小超过限制");
            return ;
        }
        _response->_version = lineArr[0];
        _response->_status = std::stoi(lineArr[1]); //"200" -> 200
        setState(HEAD_RECVING);
    }
    void HttpController::recvResponseHead(net::Buffer* buf) {
        if (_state != HEAD_RECVING) return ;
        while(true) {
            //1. 获取一行数据
            auto line = buf->getline();
            //  1. 获取行失败，但是缓冲区中数据长度过长，则返回错误
            if (!line && buf->readAbleBytes() >= KMaxLine) {
                setError("line is too long, max line is 4096");
                setOk(false);
                return;
            } else if (!line && buf->readAbleBytes() < KMaxLine) {
                LOG_DEBUG("数据量不足: %lu", buf->readAbleBytes());
                return;
            }
            // LOG_DEBUG("header: %s\n", line->c_str());
            //  3. 获取了一行数据
            if (line->empty()) {
                //空行代表头部处理结束
                setState(BODY_RECVING);
                return;
            }
            //2. 对这行数据以: 为间隔进行分割，得到key和val
            size_t pos = line->find(": ");
            if (pos == std::string::npos) {
                setError("bad request");
                setOk(false);
                return;
            }
            //3. 将key和val添加到_headers中
            std::string key = line->substr(0, pos);
            std::string val = line->substr(pos + 2);
            _response->_headers[key] = val;
        }
    }
    void HttpController::recvResponseBody(net::Buffer* buf) {
        if (_state != BODY_RECVING) return ;
        size_t totalLen = _response->getContentLength();
        if (totalLen == 0) {
            setState(RECV_COMPLATED);
            return;
        }
        size_t realLen = totalLen - _response->_body.length(); // 剩余需要接收的长度
        realLen = realLen > buf->readAbleBytes() ? buf->readAbleBytes() : realLen;
        _response->_body.append(buf->retriveAsString(realLen));
        if (_response->_body.length() == totalLen) {
            setState(RECV_COMPLATED);
            return;
        }
    }
}
}