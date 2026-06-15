#include "protocol.h"
#include "../net/server.h"

namespace proto {
    class Server {
        public:
            Server(int port)
                : _server(&_baseloop, net::InetAddress(port)) {
                _server.setConnectionCallback(
                    std::bind(&Server::onConnection, this, 
                        std::placeholders::_1));
                _server.setMessageCallback(
                    std::bind(&Server::onMessage, this, 
                        std::placeholders::_1, 
                        std::placeholders::_2, 
                        std::placeholders::_3));
            }
            ~Server() = default;
            void setProtocol(ProtocolPtr proto) {
                _protocol = proto;
            }
            void start() {
                _server.start();
                _baseloop.loop();
            }
        private:
            void onConnection(net::TcpConnectionPtr conn) {
                if (conn->connected()) {
                    _protocol->setController(conn);
                }else {
                    conn->setContext(std::any());
                }
            }
            void onMessage(net::TcpConnectionPtr conn, 
                net::Buffer* buf, net::Timestamp rtime) {
                _protocol->handleRequest(conn, buf, rtime);
            }
        private:
            net::EventLoop _baseloop;
            net::TcpServer _server;
            ProtocolPtr _protocol;
    };
}