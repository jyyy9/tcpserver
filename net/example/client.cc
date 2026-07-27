#include "../client.h"
#include <iostream>
#include "unistd.h"

void onConnection(net::TcpConnectionPtr conn) {
    if (conn->connected()) {
        std::cout << "连接建立成功!\n";
    }else {
        std::cout << "连接断开!\n";
    }
}
void onMessage(net::TcpConnectionPtr conn, 
    net::Buffer* buf, net::Timestamp recvTime) {
    auto req = buf->retriveAllAsString();
    std::cout << "收到响应:" << req << std::endl;
}

int main()
{
    // 1. 实例化一个事件循环对象
    net::EventLoopThread loopthread;
    // 2. 实例化一个服务端地址信息结构
    net::InetAddress addr("127.0.0.1", 9001);
    // 3. 实例化客户端对象
    net::TcpClient client(loopthread.startLoop(), addr);
    // 4. 为实例化的客户端对象，设置回调函数
    client.setConnectionCallback(onConnection);
    client.setMessageCallback(onMessage);
    // 5. 连接服务器
    client.connect();
    // 6. 获取客户端链接对象，与服务端进行通信
    auto connection = client.connection();
    std::cout << "=======================\n";
    for (int i = 0; i < 3; i++) {
        std::string str = "Hello World + " + std::to_string(i);
        connection->send(str);
        sleep(1);
    }
    client.disconnected();
    sleep(1);
    return 0;
}