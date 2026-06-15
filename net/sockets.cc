#include "sockets.h"
#include "log.h"
#include <cstring>
#include <errno.h>
#include<cstdio>
#include <sys/uio.h>

namespace net {
    // 创建非阻塞套接字，失败则抛异常
    int sockets::createSocket(sa_family_t family, bool block) {
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
        int ret = ::connect(sockfd, addr, sizeof(*addr));
        int errNo = ret < 0 ? errno : 0;
        if (ret < 0) {
            switch(errNo) {
                case 0: //成功的情况
                case EISCONN: //该套接字连接已完成
                case EINPROGRESS: //连接正在进行中
                    break;
                case EACCES: 
                case EPERM:  //权限不足
                case EADDRNOTAVAIL: //地址信息无效
                case EAFNOSUPPORT: // 地址域类型暂不支持
                case EBADF: //坏的文件描述符
                case EFAULT: //地址空间不够用了
                case ENOTSOCK: // 这不是一个套接字描述符
                case EPROTOTYPE: //套接字类型错误，比如使用udp连接tcp服务器
                    LOG_FATAL("连接服务器失败: %s", strerror(errno));
                    return errNo;
                case EADDRINUSE: //地址被占用
                case EAGAIN:  //资源暂时不足
                case EALREADY: //该套接字之前的连接未完成
                case ECONNREFUSED: // 连接被拒绝-服务端未启动
                case EINTR: // 阻塞操作被信号打断
                case ENETUNREACH: //网络不可达
                case ETIMEDOUT: //连接超时
                    LOG_ERROR("连接服务器失败: %s", strerror(errno));
                    break;
            }
        }
        return ret;
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
    // 转换为：192.168.1.1:8080 ->inet_ntop
    void sockets::toIpPort(char* buf, size_t size, const struct sockaddr_in* addr);
    // 转为网络字节序地址结构数据 inet_pton
    //
    void sockets::fromIpPort(const char* ip, uint16_t port, struct sockaddr_in* addr)
    {
        
    }
}