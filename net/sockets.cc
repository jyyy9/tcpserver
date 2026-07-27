#include "sockets.h"
#include "log.h"
#include <cstring>
#include <cstdio>
#include <errno.h>
#include <sys/uio.h>
#include <linux/tcp.h>

namespace net {
    // 创建非阻塞套接字，失败则抛异常
    int sockets::createSocket( bool block) {
        int32_t flag;
        if (block) {
            flag = SOCK_STREAM | SOCK_CLOEXEC;
        }else {
            flag = SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK;
        }
        int fd = ::socket(AF_INET, flag, 0);
        if (fd < 0) {
            LOG_FATAL("创建套接字失败: %s", strerror(errno));
        }
        return fd;
    }
    // 为套接字绑定地址，失败则抛异常
    void sockets::bind(int sockfd, const struct sockaddr* addr) {
        int ret = ::bind(sockfd, addr, sizeof(*addr));
        if (ret < 0) {
            LOG_FATAL("套接字绑定地址失败: %s", strerror(errno));
        }
    }
    // 直接返回系统调用即可，失败由外部处理
    int sockets::connect(int sockfd, const struct sockaddr* addr) {
        return ::connect(sockfd, addr, sizeof(*addr));
    }
    // 开始监听套接字，失败抛异常
    void sockets::listen(int sockfd) {
        int ret = ::listen(sockfd, LISTEN_SIZE);
        if (ret < 0) {
            LOG_FATAL("开始监听失败: %s", strerror(errno));
        }
    }
    // 获取新连接，accept4可以获取的同时设置套接字选项
    // 错误情况： 可接受：EAGAIN，ECONNABORTED，EINTR，EPROTO，EPERM，EMFILE
    //不可接收抛异常：EBADF，EFAULT，EINVAL，ENFILE，ENOBUFS，ENOMEM，ENOTSOCK，EOPNOTSUPP
    int sockets::accept(int sockfd, struct sockaddr_in* addr) {
        socklen_t len = sizeof(*addr);
        int newfd = accept4(sockfd, (struct sockaddr*)addr, &len, SOCK_NONBLOCK|SOCK_CLOEXEC);
        int errNo = errno;
        if (newfd < 0) {
            switch(errNo) {
                case EAGAIN: // 非阻塞的情况下没有新连接
                case ECONNABORTED: //客户端连接产生了异常
                case EINTR: //信号中断
                case EMFILE: // 进程内打开的文件描述符数量达到上限
                case EPERM: //防火墙拦截
                case EPROTO: //协议错误
                    LOG_ERROR("获取新连接失败: %s", strerror(errno));
                    break;
                case EBADF: //坏的监听套接字描述符
                case EFAULT: //地址参数错误
                case EINVAL: //无效参数-描述符还没有开始监听
                case ENFILE: //系统层面描述符数量达到上限
                case ENOMEM: //内存不足
                case ENOTSOCK: //描述符不是套接字描述符
                case EOPNOTSUPP: //当前描述符不是一个流式套接字
                    LOG_FATAL("获取新连接失败: %s", strerror(errno));
                    default:
                    LOG_FATAL("获取新连接失败: %s", strerror(errno));
                    break;

            }
        }
        return newfd;
    }
    // 直接返回系统调用
    ssize_t sockets::read(int fd, void* buf, size_t size) {
        return ::read(fd, buf, size); //  等价于 recv(fd, buf, size, 0);
    }
    // 直接返回系统调用
    ssize_t sockets::readv(int fd, struct iovec* vec, int count) {
        //readv接口叫做分块接收  缓冲区1024， 
        return ::readv(fd, vec, count);
    }
    // 直接返回系统调用
    ssize_t sockets::write(int fd, const void* buf, size_t size) {
        return ::write(fd, buf, size); // send(fd, buf, size, 0);
    }
    // 直接返回系统调用
    void sockets::close(int fd) {
        if (fd >= 0) ::close(fd);
    }
        // 转换为：网络字节序相关的信息 -》192.168.1.1:8080 
    void sockets::toIpPort(char* buf, size_t size, const struct sockaddr_in* addr) {
        // inet_ntop(int af, const void *src, char *dst, socklen_t size)
        inet_ntop(AF_INET, &addr->sin_addr.s_addr, buf, size);
        uint16_t port = ntohs(addr->sin_port);
        snprintf(buf + strlen(buf), size - strlen(buf), ":%d", port);
    }
    // 转换接口：从字符串IP地址+端口，转换为网络字节序相关的数据
    void sockets::fromIpPort(const char* ip, uint16_t port, struct sockaddr_in* addr){
        // inet_pton(AF_INET, ip, &addr->sin_addr.s_addr)
        addr->sin_addr.s_addr = inet_addr(ip);
        addr->sin_port = htons(port); //htonl
        addr->sin_family = AF_INET;
    }

    InetAddress::InetAddress(){
        std::memset(&_addr, 0, sizeof(_addr));
    }
     //提供给服务器端
    InetAddress::InetAddress(int port) {
        std::memset(&_addr, 0, sizeof(_addr));
        sockets::fromIpPort("0.0.0.0", port, &_addr);
    }
    InetAddress::InetAddress(const std::string &ip, int port) {
        std::memset(&_addr, 0, sizeof(_addr));
        sockets::fromIpPort(ip.c_str(), port, &_addr);
    }
    const struct sockaddr* InetAddress::getAddress() const {
        return (struct sockaddr*)&_addr;
    }
    void InetAddress::setAddress(struct sockaddr_in addr) {
        _addr = addr;
    }
    std::string InetAddress::toIpPort() const {
        char tmp[32];
        sockets::toIpPort(tmp, 32, &_addr);
        return tmp;
    }

    Socket::Socket(int sockfd): _sockfd(sockfd) {}
    // 关闭套接字
    Socket::~Socket() { sockets::close(_sockfd); }
    int Socket::fd() { return _sockfd; }
    void Socket::bind(const InetAddress& addr) {
        sockets::bind(_sockfd, addr.getAddress());
    }
    // 获取新连接对应描述符以及客户端的地址信息
    int Socket::accept(InetAddress* addr) {
        struct sockaddr_in peerAddr;
        int newfd = sockets::accept(_sockfd, &peerAddr);
        if (newfd >= 0) {
            addr->setAddress(peerAddr);
        }
        return newfd;
    }
    void Socket::listen() {
        return sockets::listen(_sockfd);
    }
    // IPPROTO_TCP， TCP_NODELAY
    void Socket::setTcpNoDelay(bool on) {
        // int setsockopt(int sockfd, int level, int optname, void *optval, socklen_t optlen)
        int opt = 1;
        int ret = setsockopt(_sockfd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));
        if (ret < 0) {
            LOG_ERROR("setsockopt TCP_NODELAY 错误");
        }
    }
    // SOL_SOCKET， SO_REUSEADDR
    void Socket::setReuseAddr(bool on) {
        int opt = 1;
        int ret = setsockopt(_sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        if (ret < 0) {
            LOG_ERROR("setsockopt SO_REUSEADDR 错误");
        }
    }
    // SOL_SOCKET， SO_REUSEPORT
    void Socket::setReusePort(bool on) {
        int opt = 1;
        int ret = setsockopt(_sockfd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
        if (ret < 0) {
            LOG_ERROR("setsockopt SO_REUSEPORT 错误");
        }
    }
    // SOL_SOCKET， SO_KEEPALIVE
    void Socket::setKeepAlive(bool on) {
        int opt = 1;
        int ret = setsockopt(_sockfd, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt));
        if (ret < 0) {
            LOG_ERROR("setsockopt SO_KEEPALIVE 错误");
        }
    }
}