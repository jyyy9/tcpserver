#include "protocol.h"
#include "controller.h"
#include "message.h"
#include <filesystem>

namespace proto{
namespace http {
    HttpProtocol::HttpProtocol(const std::string &wwwroot):_wwwroot(wwwroot){}
    HttpProtocol::~HttpProtocol() = default;
    // 向路由表中，添加一个get方法的，请求-回调映射信息
    void HttpProtocol::Get(const std::string& pattern, const ServerCallback &cb) {
        auto &tables = _routes["GET"];
        tables.push_back(std::make_pair(std::regex(pattern), cb));
    }
    void HttpProtocol::Post(const std::string& pattern, const ServerCallback &cb){
        auto &tables = _routes["POST"];
        tables.push_back(std::make_pair(std::regex(pattern), cb));
    }
    void HttpProtocol::Put(const std::string& pattern, const ServerCallback &cb){
        auto &tables = _routes["PUT"];
        tables.push_back(std::make_pair(std::regex(pattern), cb));
    }
    void HttpProtocol::Delete(const std::string& pattern, const ServerCallback &cb){
        auto &tables = _routes["DELETE"];
        tables.push_back(std::make_pair(std::regex(pattern), cb));
    }
    // ...
    void HttpProtocol::setController(net::TcpConnectionPtr conn) {
        std::shared_ptr<HttpController> cntl(new HttpController);
        conn->setContext(cntl);
    }
    // 请求处理-- 用于服务端
    // void HttpProtocol::handleRequest(net::TcpConnectionPtr, 
    //     net::Buffer*, net::Timestamp)
    // 响应处理-- 用于客户端
    // void HttpProtocol::handleResponse(net::TcpConnectionPtr, 
    //     net::Buffer*, net::Timestamp) = 0;
    void HttpProtocol::dispatch(ControllerPtr cntl) {
        HttpControllerPtr httpCntl = std::dynamic_pointer_cast<HttpController>(cntl);
        assert(httpCntl);
        //静态资源处理
        bool ret = fileHandler(httpCntl);
        if (ret == true) return;
        //动态请求处理
        ret = routeHandler(httpCntl);
        //上述两个都没有处理成功，则返回404
        if (ret == true) return;
        httpCntl->setStatus(404);
    }
    
    bool HttpProtocol::fileHandler(HttpControllerPtr cntl) {
        // 1. 将请求的i资源路径转换为实际的文件存储路径
        //      将相对根目录转换为实际的存储路径
        auto request = cntl->request();
        auto response = cntl->response();
        std::string realpath = _wwwroot + request->getPath();
        // 2. 转换后的文件路径，是否是一个目录则返回false
        if (std::filesystem::is_directory(realpath) == true) {
            return false;
        }
        // 3. 读取文件数据，到response的_body中
        bool ret = readFile(realpath, response->body());
        if (ret == false) {
            return false;
        }
        // index.html  ->  text/html   jpeg ->  image/jpeg
        // 4. 设置响应正文类型
        //   1. 获取文件的扩展名 -》 获取mime -》设置正文类型
        std::string ext = std::filesystem::path(realpath).extension();
        std::string contentType = getMime(ext);
        response->setHeader("Content-Type", contentType);
        return true;
    }
    bool HttpProtocol::routeHandler(HttpControllerPtr cntl) {
        auto request = cntl->request();
        auto response = cntl->response();
        // 1. 获取到请求的path和method
        std::string path = request->getPath();
        std::string method = request->getMethod();
        // 2. 找到对应method的路由表
        auto it = _routes.find(method);
        if (it == _routes.end()) {
            response->setStatus(405);
            return false;
        }
        const auto &tables = it->second;
        // 3. 遍历路由表，针对path进行路径匹配，
        for (auto &tbl : tables) {
            //匹配成功，则使用回调函数，进行处理，处理完毕后正确返回
            std::smatch &match = request->getMatch();
            // regex_match(src,  smatch,   regex)
            bool ret = std::regex_match(path, match, tbl.first);
            if (ret == true) {
                auto func = tbl.second;
                func(request, response);
                return true;
            }
            //匹配失败则尝试下一个映射关系的匹配
        }
        // 4. 路径匹配全部失败了，返回false
        return false;
    }
}
}