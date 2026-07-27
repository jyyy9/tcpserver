#include "protocol.h"
#include "../net/client.h"

namespace proto {
    class Client {
        public:
            Client(net::EventLoop* loop, const net::InetAddress& addr);
            void setProtocol(ProtocolPtr protocol) {
                _protocol = protocol;
            }
            // 连接服务器,确保连接已经建立完成，connection各项回调函数已经设置完毕
            void connect() {
                _client.connect();
                (void)_client.connection(); 
            }
            // _protocol->send(request, callback);
        private:
            void onConnection(net::TcpConnectionPtr conn) {
                if (conn->connected()) {
                    _protocol->setController(conn);
                    //给protocol设置发送数据时使用的连接对象
                    _protocol->setConnection(conn); 
                }else {
                    conn->setContext(std::any());
                }
            }
            void onMessage(net::TcpConnectionPtr conn, 
                net::Buffer* buf, net::Timestamp rtime) {
                _protocol->handleRequest(conn, buf, rtime);
            }
        private:
            net::TcpClient _client; //网络通信客户端 // connection
            ProtocolPtr _protocol; //具体的应用层协议对象
    };
}