#include "poller.h"
#include "timestamp.h"
#include "log.h"
#include "channel.h"
#include <sys/epoll.h>
#include <cstring>
#include <cassert>
#include <errno.h>
#include <unistd.h>

namespace net {
    
    int createEpoll() {
        int fd = ::epoll_create1(EPOLL_CLOEXEC);
        if (fd < 0) {
            LOG_FATAL("创建epoll失败: %s", strerror(errno));
        }
        return fd;
    }

    PollerPtr Poller::defaultPoller(){
        std::shared_ptr<EpollPoller> ptr(new EpollPoller());
        return ptr;
    }
    // 针对成员进行初始化_epfd
    EpollPoller::EpollPoller() 
        : _epfd(createEpoll())
        , _evs(DETAULT_EVENT_SIZE) {}
    EpollPoller::~EpollPoller() { ::close(_epfd); }

    Timestamp EpollPoller::wait(std::vector<Channel*> &actives) {
        // int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);
        actives.clear();
        int ret = epoll_wait(_epfd, &_evs[0], _evs.size(), EPOLL_TIMEOUT);
        if (ret < 0) {
            LOG_ERROR("epoll_wait出错: %s", strerror(errno));
            return;
        } else if (ret == 0) {
            LOG_DEBUG("EPOLL WAIT 超时");
            return;
        }
        for (int i = 0; i < ret; ++i) {
            Channel* channel = (Channel*)_evs[i].data.ptr;
            channel->setRevents(_evs[i].events);
        }
        if (ret == _evs.size()) { //就绪事件数组打满了
            _evs.resize(_evs.size() * 2);
        }
    }
    const char* EpollPoller::eventStr(int op) {
        switch(op){
            case EPOLL_CTL_ADD: return "EPOLL_CTL_ADD";
            case EPOLL_CTL_MOD: return "EPOLL_CTL_MOD";
            case EPOLL_CTL_DEL: return "EPOLL_CTL_DEL";
        }
        LOG_FATAL("epoll监控类型错误: %d", op);
    }
    void EpollPoller::update(int op, Channel *channel) {
        // 实际实现epoll_ctl相关的操作
        // 1. 获取channel中的fd和events
        int fd = channel->fd();
        // 2. 使用epoll_ctl进行事件操作
        struct epoll_event ev;
        ev.data.ptr = channel;
        ev.events = channel->events();
        int ret = epoll_ctl(_epfd, op, fd, &ev);
        if (ret < 0) {
            LOG_ERROR("epoll_ctl出错: %s-%s", eventStr(op), strerror(errno));
        }
    }
    void EpollPoller::updateChannel(Channel *channel) {
        // 要对channel中的描述符，根据不同的状态进行添加/修改/解除epoll监控操作
        //  kNew： 新增监控&新增管理； 
        //  kAdded：修改要监控的事件；  
        //  kDeleted：表示描述符没有监控任何事件，但是以前添加过管理，新增监控
        ChannelState state = channel->state();
        int fd = channel->fd();
        if (state == kNew || state == kDeleted) {
            if (state == kNew) {
                assert(_channels.find(fd) == _channels.end());
                _channels.insert(std::make_pair(channel->fd(), channel));
            }else {
                assert(_channels.find(fd) != _channels.end());
                assert(_channels[fd] == channel);
            }
            update(EPOLL_CTL_ADD, channel);
            channel->setState(kAdded);
        }else {
            // 监控中的描述符：若当前要监控的事件是0，就代表要解除监控； 否则就是修改要监控的事件
            if (channel->events() == kNoneEvent) {
                update(EPOLL_CTL_DEL, channel);
                channel->setState(kDeleted);
            }else {
                update(EPOLL_CTL_MOD, channel);
            }
        }
    }
    void EpollPoller::removeChannel(Channel *channel) {
        int fd = channel->fd();
        assert(_channels.find(fd) != _channels.end());
        assert(_channels[fd] == channel);
        // 1. 解除epoll事件监控
        if (channel->state() == kAdded) {
            update(EPOLL_CTL_DEL, channel);
        }
        // 2. 解除channel管理
        _channels.erase(fd);
        channel->setState(kNew);
    }
}