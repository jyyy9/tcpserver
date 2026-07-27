#include "message.h"
#include "controller.h"

namespace proto{
namespace http {

    HttpRequest::HttpRequest() {}
    HttpRequest::~HttpRequest() = default;
    // 成员设置，成员获取
    // 设置请求方法
    void HttpRequest::setMethod(std::string method) { _method = method; }
    // 获取请求方法
    const std::string& HttpRequest::getMethod() const { return _method; }
    // 设置请求路径
    void HttpRequest::setPath(const std::string &path) { 
        urlEncode(path, _path, false);
        // LOG_DEBUG("SET PATH:  %s\n", _path.c_str());
    }
    // 获取请求路径
    const std::string& HttpRequest::getPath() const { return _path; }
    // 设置请求版本
    void HttpRequest::setVersion(std::string version) { _version = version; }
    // 获取请求版本
    const std::string& HttpRequest::getVersion() const { return _version; }
    // 设置请求正文
    void HttpRequest::setBody(std::string body) { _body = body; }
    // 获取请求正文
    const std::string& HttpRequest::getBody() const { return _body; }
    // 设置路径匹配结果
    void HttpRequest::setMatch(std::smatch match) { _match = match; }
    // 获取路径匹配结果
    const std::smatch& HttpRequest::getMatch() const { return _match; }
    // 获取请求正文长度
    size_t HttpRequest::getContentLength()  { 
        if (_headers.find("Content-Length") == _headers.end())
            return 0;
        return std::stoi(_headers["Content-Length"]); 
    }
    // 判断当前是否是长连接
    bool HttpRequest::isKeepAlive()  { 
        auto it = _headers.find("Connection");
        if (it == _headers.end()) {
            if (_version == "HTTP/1.0") {
                return false;
            }
        }
        return true;
    }
    // 设置请求头
    void HttpRequest::setHeader(std::string key, std::string value) { 
        _headers[key] = value; 
    }
    std::optional<std::string> HttpRequest::header(const std::string &key) {
        auto it = _headers.find(key);
        if (it == _headers.end()) {
            return std::nullopt;
        }
        return it->second;
    }
    // 获取请求头
    const std::unordered_map<std::string, std::string>& HttpRequest::getHeaders() const { 
        return _headers; 
    }
    // 获取查询参数
    const std::unordered_map<std::string, std::string>& HttpRequest::getQuery() const { 
        return _query; 
    }
    // 设置查询参数
    void HttpRequest::setQuery(const std::string &key, const std::string &val) { 
        std::string ekey, eval;
        urlEncode(key, ekey, true);
        urlEncode(val, eval, true);
        
        // LOG_DEBUG("SET PARAMS:  %s = %s\n", ekey.c_str(), eval.c_str());
        _query[ekey] = eval; 
    }
    

    HttpResponse::HttpResponse() : _status(200){}
    HttpResponse::~HttpResponse() = default;
    void HttpResponse::setStatus(int status) { _status = status; }
    int HttpResponse::status() { return _status; }
    void HttpResponse::setVersion(const std::string &version) { 
        _version = version; 
    }
    const std::string& HttpResponse::version() { return _version; }
    void HttpResponse::setBody(const std::string& body, const std::string &ct) {
        _body = body;
        _headers["Content-Type"] = ct;
    }
    const std::string& HttpResponse::body() const { return _body; }
    std::string& HttpResponse::body() { return _body; }
    void HttpResponse::setHeader(const std::string& key, const std::string &val) {
        _headers[key] = val;
    }
    std::optional<std::string> HttpResponse::header(const std::string &key) {
        auto it = _headers.find(key);
        if (it == _headers.end()) {
            return std::nullopt;
        }
        return it->second;
    }
    const std::unordered_map<std::string, std::string>& HttpResponse::headers() const {
        return _headers;
    }
    size_t HttpResponse::getContentLength()  { 
        if (_headers.find("Content-Length") == _headers.end())
            return 0;
        return std::stoi(_headers["Content-Length"]); 
    }
    bool HttpResponse::isKeepAlive()  { 
        auto it = _headers.find("Connection");
        if (it == _headers.end()) {
            if (_version == "HTTP/1.0") {
                return false;
            }
        }
        return true;
    }
}
}