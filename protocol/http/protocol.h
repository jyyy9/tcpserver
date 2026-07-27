#include "../protocol.h"
#include "controller.h"
#include "message.h"
#include <regex>

namespace proto {
namespace http {
    class HttpProtocol : public Protocol {
        public:
            using ServerCallback = std::function<void(const HttpRequestPtr, HttpResponsePtr)>;
            HttpProtocol(const std::string &wwwroot = "./wwwroot");
            ~HttpProtocol();
            // 向路由表中，添加一个get方法的，请求-回调映射信息
            void Get(const std::string& pattern, const ServerCallback &cb);
            void Post(const std::string& pattern, const ServerCallback &cb);
            void Put(const std::string& pattern, const ServerCallback &cb);
            void Delete(const std::string& pattern, const ServerCallback &cb);
            // ...
            virtual void setController(net::TcpConnectionPtr conn) override;
            virtual ControllerPtr getController(net::TcpConnectionPtr conn) override;
            virtual void dispatchRequest(ControllerPtr) override;
            virtual void dispatchResponse(ControllerPtr) override;
            virtual void setConnection(net::TcpConnectionPtr) override;
        private:
            bool fileHandler(HttpControllerPtr);
            bool routeHandler(HttpControllerPtr);
        private:
            std::string _wwwroot; //静态资源根目录
            //动态请求路由表: <请求方法，指定方法路由函数表>
            using Handlers = std::vector<std::pair<std::regex, ServerCallback>>;
            std::unordered_map<std::string, Handlers> _routes;

    };
}
}