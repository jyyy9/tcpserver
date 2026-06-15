/*
    从Message派生出HTTP请求以及响应消息类
*/

#include <sstream>
#include <unordered_map>
#include <regex>
#include <string>
#include <memory>
#include <cassert>
#include "../protocol.h"
#include "controller.h"
#include "utils.h"

namespace proto {
namespace http {
    class Request;
    class Response;
    using RequestPtr = std::shared_ptr<Request>;
    using ResponsePtr = std::shared_ptr<Response>;
    const static int KMaxLine = 4096;
    enum State {
        NEW = 0, 
        LINE_RECVING, //首行接收中
        HEAD_RECVING, //头部字段接收中
        BODY_RECVING, //正文接收中
        RECV_COMPLATED, //接收完成
        LINE_SENDING, //首行发送中
        HEAD_SENDING, //头部字段发送中
        BODY_SENDING, //正文发送中
        SEND_COMPLATED //发送完成
    };
    class HttpRequest : public proto::Message {
        public:
            HttpRequest():_state(NEW) {}
            ~HttpRequest() = default;
            virtual bool isRecvComplete() const { return _state == RECV_COMPLATED; }
            virtual bool isSendComplete() const { return _state == SEND_COMPLATED; }
            // 成员设置，成员获取
            // 设置请求方法
            void setMethod(std::string method) { _method = method; }
            // 获取请求方法
            const std::string& getMethod() const { return _method; }
            // 设置请求路径
            void setPath(const std::string &path) { 
                urlEncode(path, _path, false);
            }
            // 获取请求路径
            const std::string& getPath() const { return _path; }
            // 设置请求版本
            void setVersion(std::string version) { _version = version; }
            // 获取请求版本
            const std::string& getVersion() const { return _version; }
            // 设置请求正文
            void setBody(std::string body) { _body = body; }
            // 获取请求正文
            const std::string& getBody() const { return _body; }
            // 设置路径匹配结果
            void setMatch(std::smatch match) { _match = match; }
            // 获取路径匹配结果
            const std::smatch& getMatch() const { return _match; }
            // 获取请求正文长度
            size_t getContentLength() const { 
                if (_headers.find("Content-Length") == _headers.end())
                    return 0;
                return std::stoi(_headers["Content-Length"]); 
            }
            // 判断当前是否是长连接
            bool isKeepAlive() const { return _headers["Connection"] == "keep-alive"; }
            // 设置请求头
            void setHeader(std::string key, std::string value) { 
                _headers[key] = value; 
            }
            // 获取请求头
            const std::unordered_map<std::string, std::string>& getHeaders() const { 
                return _headers; 
            }
            // 获取查询参数
            const std::unordered_map<std::string, std::string>& getQuery() const { 
                return _query; 
            }
            // 设置查询参数
            void setQuery(const std::string &key, const std::string &val) { 
                std::string ekey, eval;
                urlEncode(key, ekey, true);
                urlEncode(val, eval, true);
                _query[ekey] = eval; 
            }
            virtual void recvData(Controller* controller, net::Buffer* buffer) {
                HttpController* httpCntl = dynamic_cast<HttpController*>(controller);
                assert(httpCntl != nullptr);
                switch (_state) {
                    case NEW:
                        setState(LINE_RECVING);
                    case LINE_RECVING:
                        recvLine(httpCntl, buffer);
                        if(httpCntl->isOk() == false) break;
                    case HEAD_RECVING:
                        recvHead(httpCntl, buffer);
                        if(httpCntl->isOk() == false) break;
                    case BODY_RECVING:
                        recvBody(httpCntl, buffer);
                        if(httpCntl->isOk() == false) break;
                }
            }
            virtual void sendData(Controller* controller, net::TcpConnectionPtr conn) {
                HttpController* httpCntl = dynamic_cast<HttpController*>(controller);
                assert(httpCntl != nullptr);
                switch (_state) {
                    case NEW:
                        setState(LINE_SENDING);
                    case LINE_SENDING:
                        sendLine(httpCntl, conn);
                        if(httpCntl->isOk() == false) break;
                    case HEAD_SENDING:
                        sendHead(httpCntl, conn);
                        if(httpCntl->isOk() == false) break;
                    case BODY_SENDING:
                        sendBody(httpCntl, conn);
                        if(httpCntl->isOk() == false) break;
                }
            }
        private:
            void setState(State state) { _state = state; }
            void recvLine(HttpController* controller, net::Buffer* buffer) {
                // GET /path?query=value HTTP/1.1  请求首行的处理
                auto line = buffer->getline();
                // 缓冲区中有数据，但是不足一行，但是数据量超过最大行长度
                if(!line && buffer->readAbleBytes() > KMaxLine) {
                    controller->setError("line is too long, max line is 4096");
                    controller->setOk(false);
                    controller->setStatus(414); // 414 URI Too Long
                    return;
                }else if(buffer->readAbleBytes() < KMaxLine) {
                    //当前首行数据不足
                    return;
                }
                // GET /path?query=value HTTP/1.1

                std::regex re(R"(^([A-Z]+)\s+(/[^?#]*)(\?([^#\s]*))?\s+HTTP/(\d+\.\d+)$)");
                std::smatch match;
                if (std::regex_match(*line, match, re)) {
                    setMethod(match[1].str());
                    setPath(match[2].str());
                    std::string params = match[4].str(); // 查询参数：key=val&key2=val2
                    setVersion(match[5].str());
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
                            controller->setOk(false);
                            controller->setError("查询字符串解析失败");
                            controller->setStatus(400);
                            return;
                        }
                        ret = urlDecode(val, dval, true);
                        if(ret == false) {
                            controller->setOk(false);
                            controller->setError("查询字符串解析失败");
                            controller->setStatus(400);
                            return;
                        }
                        setQuery(dkey, dval);
                    }
                }else {
                    controller->setError("line is not http request line");
                    controller->setOk(false);
                    controller->setStatus(400); // 400 Bad Request
                    return;
                }
                setState(HEAD_RECVING); // 切换到请求头接收状态
            }
            void recvHead(HttpController* controller, net::Buffer* buffer) {
                if (_state != HEAD_RECVING) return ;
                //key: val\r\nkey: val\r\n....
                while(true) {
                    //1. 获取一行数据
                    auto line = buffer->getline();
                    //  1. 获取行失败，但是缓冲区中数据长度过长，则返回错误
                    if (!line && buffer->readAbleBytes() >= KMaxLine) {
                        controller->setError("line is too long, max line is 4096");
                        controller->setOk(false);
                        controller->setStatus(414); // 414 URI Too Long
                        return;
                    }
                    //  2. 获取行失败，但是缓冲区中数据长度没有超过限制，当前只是数据不足，则正确返回
                    else if (buffer->readAbleBytes() < KMaxLine) {
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
                        controller->setError("bad request");
                        controller->setOk(false);
                        controller->setStatus(400);
                        return;
                    }
                    //3. 将key和val添加到_headers中
                    std::string key = line->substr(0, pos);
                    std::string val = line->substr(pos + 2);
                    _headers[key] = val;
                }
            }
            void recvBody(HttpController* controller, net::Buffer* buffer) {
                if (_state != BODY_RECVING) return ;
                // 从缓冲区中取出数据，放到_body中
                // 1. 通过头部字段中的Content-Length获取正文长度
                size_t totalLen = getContentLength();
                if (totalLen == 0) {
                    setState(RECV_COMPLATED);
                    return;
                }
                // 2. 若缓冲区中数据长度大于正文长度则只获取正文长度数据到body中
                // 3. 若_body中的数据长度，等于Content-Length，则正文接收完毕，设置状态
                size_t realLen = totalLen - _body.length(); // 剩余需要接收的长度
                realLen = realLen > buffer->readAbleBytes() ? buffer->readAbleBytes() : realLen;
                _body.append(buffer->retriveAsString(realLen));
                if (_body.length() == totalLen) {
                    setState(RECV_COMPLATED);
                    return;
                }
            }
            
            void sendLine(HttpController* controller, net::TcpConnectionPtr conn) {
                // GET /path?key=val&key=val HTTP/1.1\r\n
                std::stringstream ss;
                ss << _method << " ";
                ss << _path;
                if (_query.empty() == true) {
                    std::stringstream param_ss;
                    param_ss << "?";
                    for (auto &param : _query) {
                        param_ss << param.first << "=" << param.second << "&";
                    }
                    std::string str = param_ss.str();
                    if (str.back() == '&') str.pop_back();
                    ss << str;
                }
                ss << " ";
                ss << "HTTP/" << _version;
                ss << "\r\n";
                conn->send(ss.str());
                setState(HEAD_SENDING);
            }
            void sendHead(HttpController* controller, net::TcpConnectionPtr conn) {

            }
            void sendBody(HttpController* controller, net::TcpConnectionPtr conn) {

            }
        private:
            State _state;// 状态
            std::string _method;// 请求方法
            std::string _path;// 请求路径
            std::string _version;// 请求版本
            std::string _body;// 请求体
            std::smatch _match;// 路径匹配结果
            std::unordered_map<std::string, std::string> _headers;// 请求头
            std::unordered_map<std::string, std::string> _query;// 查询参数
    };
}
}