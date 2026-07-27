#include "../message.h"

int main()
{
    proto::http::HttpController cntl;
    proto::http::HttpResponse response;
    net::Buffer buffer;
    buffer.append("HTTP/1.1 200 OK\r\nContent-Length: 11\r\nConnection: close\r\n\r\nhello world");
    response.recvData(&cntl, &buffer);
    std::cout << response.version() << std::endl;
    std::cout << response.status() << std::endl;
    std::cout << response.body() << std::endl;
    auto & headers = response.headers();
    for (auto &h : headers){
        std::cout << h.first << " " << h.second << std::endl;
    }
    // response.setStatus(200);
    // response.setVersion("HTTP/1.1");
    // response.setHeader("Connection", "close");
    // response.setBody("hello world", "text/plain");
    // response.sendData(&cntl, nullptr);



    // proto::http::HttpRequest  request;
    // net::Buffer buffer;
    // proto::http::HttpController cntl;
    // buffer.append("POST /login?name=C%2b%2b&age=10 HTTP/1.1\r\nHost: 127.0.0.1:80\r\nConnection: close\r\n\r\n");
    // // auto str = buffer.retriveAllAsString();
    // // std::cout << str << std::endl;

    // request.recvData(&cntl, &buffer);
    // if (cntl.isOk() == false) {
    //     std::cout << cntl.getError() << std::endl;
    // }
    // std::cout << request.getMethod() << std::endl;
    // std::cout << request.getVersion() << std::endl;
    // std::cout << request.getPath() << std::endl;
    // auto & headers = request.getHeaders();
    // for (auto &h : headers){
    //     std::cout << h.first << " " << h.second << std::endl;
    // }
    // auto & query = request.getQuery();
    // for (auto &h : query){
    //     std::cout << h.first << " " << h.second << std::endl;
    // }
    // request.setMethod("Get");
    // request.setPath("/hello");
    // request.setQuery("name", "C++");
    // request.setQuery("age", "20");
    // request.setVersion("1.1");
    // request.sendData(nullptr, nullptr);

    return 0;
}