#include "eventloop.h"
#include "channel.h"
#include "log.h"

namespace net {
    Channel::Channel(int fd, EventLoop* loop)
        : _loop(loop)
        , _fd(fd)
        , _state(kNew)
        , _events(0)
        , _revents(0)
        , _tied(false) 
        , _addedToLoop(false)
        , _eventHandling(false){
        LOG_DEBUG("construct Channel: %p", this);
    }
    Channel::~Channel() {
        LOG_DEBUG("desstruct Channel: %p", this);
    }
    //解除监控，并移除管理 
    void Channel::remove() {
        _loop->removeChannel(this);
    }
    void Channel::update() {
        _loop->updateChannel(this);
    }
}