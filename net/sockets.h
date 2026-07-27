#pragma once 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

namespace net {
 namespace sockets {
        const static int LISTEN_SIZE = 1000;
        // 创建非阻塞套接字，失败则抛异常
        int createSocket(bool block = false);
        // 为套接字绑定地址，失败则抛异常
        void bind(int sockfd, const struct sockaddr* addr);
        // 直接返回系统调用即可，失败由外部处理
        int connect(int sockfd, const struct sockaddr* addr);
        // 开始监听套接字，失败抛异常
        void listen(int sockfd);
        // 获取新连接，accept4可以获取的同时设置套接字选项
        // 错误情况： 可接受：EAGAIN，ECONNABORTED，EINTR，EPROTO，EPERM，EMFILE
        //不可接收抛异常：EBADF，EFAULT，EINVAL，ENFILE，ENOBUFS，ENOMEM，ENOTSOCK，EOPNOTSUPP
        int accept(int sockfd, struct sockaddr_in* addr);
        // 直接返回系统调用
        ssize_t read(int fd, void* buf, size_t size);
        // 直接返回系统调用
        ssize_t readv(int fd, struct iovec* vec, int count);
        // 直接返回系统调用
        ssize_t write(int fd, const void* buf, size_t size);
        // 直接返回系统调用
        void close(int fd);
        // 转换为：192.168.1.1:8080 inet_ntop
        void toIpPort(char* buf, size_t size, const struct sockaddr_in* addr);
        // 转为网络字节序地址结构数据 inet_pton
        void fromIpPort(const char* ip, uint16_t port, struct sockaddr_in* addr);
    }

    // 封装一个地址信息的操作类：方便我们后边对地址进行操作
    class InetAddress {
        public:
            InetAddress();
            InetAddress(int port); //提供给服务器端
            InetAddress(const std::string &ip, int port);
            const struct sockaddr* getAddress() const;
            void setAddress(struct sockaddr_in addr);
            std::string toIpPort() const;
        private:
            struct sockaddr_in _addr;//IPv4的地址结构对象
    };

    // 套接字操作的封装
    class Socket {
        public:
            // 初始化成员
            Socket(int sockfd);
            // 关闭套接字
            ~Socket();
            int fd();
            void bind(const InetAddress& addr);
            // 获取新连接对应描述符以及客户端的地址信息
            int accept(InetAddress* addr);
            void listen();
            // IPPROTO_TCP， TCP_NODELAY
            void setTcpNoDelay(bool on);
            // SOL_SOCKET， SO_REUSEADDR
            void setReuseAddr(bool on);
            // SOL_SOCKET， SO_REUSEPORT
            void setReusePort(bool on);
            // SOL_SOCKET， SO_KEEPALIVE
            void setKeepAlive(bool on);
        private:
            int _sockfd; //套接字描述符
    };
}