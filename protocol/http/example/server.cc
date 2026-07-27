#include "../../server.h"
#include "../protocol.h" //httpProtocol

#include <sstream>
#include <unordered_map>
#include <vector>

using namespace proto::http;

static std::unordered_map<std::string, std::string> g_userDatabase;

static bool parseFormBody(const std::string &body,
    std::unordered_map<std::string, std::string> &fields) {
    std::vector<std::string> pairs;
    split(body, "&", pairs);
    for (const auto &pair : pairs) {
        auto pos = pair.find('=');
        if (pos == std::string::npos) {
            continue;
        }
        std::string key;
        std::string value;
        if (!urlDecode(pair.substr(0, pos), key, true) ||
            !urlDecode(pair.substr(pos + 1), value, true)) {
            return false;
        }
        fields[key] = value;
    }
    return true;
}

static std::string escapeJson(const std::string &value) {
    std::ostringstream out;
    for (char c : value) {
        switch (c) {
            case '\\': out << "\\\\"; break;
            case '"': out << "\\\""; break;
            case '\b': out << "\\b"; break;
            case '\f': out << "\\f"; break;
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    out << "\\u" << std::hex << std::uppercase
                        << (int)static_cast<unsigned char>(c);
                } else {
                    out << c;
                }
        }
    }
    return out.str();
}

static std::string makeJsonResponse(bool success, const std::string &message) {
    std::ostringstream json;
    json << "{\"success\":" << (success ? "true" : "false")
         << ",\"message\":\"" << escapeJson(message) << "\"}";
    return json.str();
}

void hello(const proto::http::HttpRequestPtr req,
    proto::http::HttpResponsePtr rsp) {
    (void)req;
    std::cout << "收到一个 hello 请求\n";
    rsp->setBody("欢迎使用 HTTP 登录注册示例！", "text/plain; charset=utf-8");
}

void login(const proto::http::HttpRequestPtr req,
    proto::http::HttpResponsePtr rsp) {
    std::unordered_map<std::string, std::string> form;
    if (!parseFormBody(req->getBody(), form)) {
        rsp->setStatus(400);
        rsp->setBody(makeJsonResponse(false, "请求参数解析失败"), "application/json; charset=utf-8");
        return;
    }

    auto itUsername = form.find("username");
    auto itPassword = form.find("password");
    if (itUsername == form.end() || itPassword == form.end() ||
        itUsername->second.empty() || itPassword->second.empty()) {
        rsp->setStatus(400);
        rsp->setBody(makeJsonResponse(false, "用户名和密码不能为空"), "application/json; charset=utf-8");
        return;
    }

    const std::string &username = itUsername->second;
    const std::string &password = itPassword->second;
    LOG_DEBUG("%s - %s", username.c_str(), password.c_str());
    auto itUser = g_userDatabase.find(username);
    if (itUser == g_userDatabase.end() || itUser->second != password) {
        rsp->setStatus(401);
        rsp->setBody(makeJsonResponse(false, "用户名或密码错误"), "application/json; charset=utf-8");
        return;
    }

    rsp->setStatus(200);
    rsp->setBody(makeJsonResponse(true, "登录成功"), "application/json; charset=utf-8");
}

void registry(const proto::http::HttpRequestPtr req,
    proto::http::HttpResponsePtr rsp) {
    std::unordered_map<std::string, std::string> form;
    if (!parseFormBody(req->getBody(), form)) {
        rsp->setStatus(400);
        rsp->setBody(makeJsonResponse(false, "请求参数解析失败"), "application/json; charset=utf-8");
        return;
    }

    auto itUsername = form.find("username");
    auto itPassword = form.find("password");
    auto itConfirm = form.find("confirm_password");
    if (itUsername == form.end() || itPassword == form.end() || itConfirm == form.end() ||
        itUsername->second.empty() || itPassword->second.empty() || itConfirm->second.empty()) {
        rsp->setStatus(400);
        rsp->setBody(makeJsonResponse(false, "请填写完整的注册信息"), "application/json; charset=utf-8");
        return;
    }

    const std::string &username = itUsername->second;
    const std::string &password = itPassword->second;
    const std::string &confirmPassword = itConfirm->second;

    if (password != confirmPassword) {
        rsp->setStatus(400);
        rsp->setBody(makeJsonResponse(false, "两次密码输入不一致"), "application/json; charset=utf-8");
        return;
    }
    if (password.size() < 6) {
        rsp->setStatus(400);
        rsp->setBody(makeJsonResponse(false, "密码长度至少 6 位"), "application/json; charset=utf-8");
        return;
    }
    if (g_userDatabase.find(username) != g_userDatabase.end()) {
        rsp->setStatus(409);
        rsp->setBody(makeJsonResponse(false, "该用户名已被注册"), "application/json; charset=utf-8");
        return;
    }

    g_userDatabase[username] = password;
    rsp->setStatus(201);
    rsp->setBody(makeJsonResponse(true, "注册成功，请登录"), "application/json; charset=utf-8");
}

int main()
{
    // 实例化一个HTTP协议对象，并注册路由信息
    auto proto = std::make_shared<proto::http::HttpProtocol>("./wwwroot");
    proto->Get("/hello", hello);
    proto->Post("/login", login);
    proto->Post("/registry", registry); // 兼容旧路径
    proto->Post("/register", registry);
    // 实例化一个服务器对象，并设置应用协议
    proto::Server server(8080);
    server.setProtocol(proto);
    // 启动服务器
    server.start();
    return 0;
}