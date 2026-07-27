/*
    从Message派生出HTTP请求以及响应消息类
*/
#pragma once
#include <sstream>
#include <unordered_map>
#include <regex>
#include <string>
#include <memory>
#include <cassert>
#include <optional>
#include "../protocol.h"
#include "utils.h"

namespace proto {
namespace http {
    class HttpRequest {
        public:
            HttpRequest();
            ~HttpRequest();
            // 成员设置，成员获取
            // 设置请求方法
            void setMethod(std::string method);
            // 获取请求方法
            const std::string& getMethod() const;
            // 设置请求路径
            void setPath(const std::string &path);
            // 获取请求路径
            const std::string& getPath() const;
            // 设置请求版本
            void setVersion(std::string version);
            // 获取请求版本
            const std::string& getVersion() const;
            // 设置请求正文
            void setBody(std::string body);
            // 获取请求正文
            const std::string& getBody() const;
            // 设置路径匹配结果
            void setMatch(std::smatch match);
            // 获取路径匹配结果
            const std::smatch& getMatch() const;
            std::smatch& getMatch() { return _match; }
            // 获取请求正文长度
            size_t getContentLength();
            // 判断当前是否是长连接
            bool isKeepAlive() ;
            // 设置请求头
            void setHeader(std::string key, std::string value);
            std::optional<std::string> header(const std::string &key);
            // 获取请求头
            const std::unordered_map<std::string, std::string>& getHeaders() const;
            // 获取查询参数
            const std::unordered_map<std::string, std::string>& getQuery() const;
            // 设置查询参数
            void setQuery(const std::string &key, const std::string &val);
        public:
            std::string _host; //目标服务器
            std::string _method;// 请求方法
            std::string _path;// 请求路径
            std::string _version;// 请求版本
            std::string _body;// 请求体
            std::smatch _match;// 路径匹配结果  // /numbers/1234  /numbers/(\d+) 
            std::unordered_map<std::string, std::string> _headers;// 请求头
            std::unordered_map<std::string, std::string> _query;// 查询参数
    };

    class HttpResponse {
        public:
            HttpResponse();
            ~HttpResponse();
            void setStatus(int status);
            int status();
            void setVersion(const std::string &version);
            const std::string& version();
            void setBody(const std::string& body, const std::string &ct="text/html");
            const std::string& body() const;
            std::string& body();
            void setHeader(const std::string& key, const std::string &val);
            std::optional<std::string> header(const std::string &key);
            const std::unordered_map<std::string, std::string>& headers() const;
            size_t getContentLength();
            bool isKeepAlive();

        public:
            int _status; //响应状态码
            std::string _version;// 协议版本  HTTP/1.1
            std::string _body; //响应正文
            std::unordered_map<std::string, std::string> _headers;// 请求头
    };
    using HttpRequestPtr = std::shared_ptr<HttpRequest>;
    using HttpResponsePtr = std::shared_ptr<HttpResponse>;
}
}