#include "eventloop.h"
#include "channel.h"
#include "log.h"
#include <cassert>

namespace net {
    Channel::Channel(int fd, EventLoop* loop)
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
        //LOG_DEBUG("construct Channel: %p", this);
    }
    Channel::~Channel() {
        assert(_addedToLoop == false); //断言当前channel已经解除了监控
        assert(_eventHandling == false); //当前channel必须没在事件处理中
        //LOG_DEBUG("desstruct Channel: %p", this);
    }
    //解除监控，并移除管理 
    void Channel::remove() {
         // LOG_DEBUG("channel remove: %d", _fd);
        _addedToLoop = false;
        _loop->removeChannel(this);
    }
    void Channel::update() {
        _addedToLoop = true;
        _loop->updateChannel(this);
    }
}