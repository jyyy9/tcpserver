#include "eventloop.h"
#include "channel.h"
#include "log.h"
#include <unistd.h>
#include <cstring>
#include <errno.h>
#include <sys/eventfd.h>

namespace net {
    int createEventFd() {
        // int eventfd(unsigned int initval, int flags);
        int fd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
        if (fd < 0) {
            LOG_FATAL("创建 eventfd 失败: %s", strerror(errno));
        }
        return fd;
    }
    void writeEventFd(int efd) {
        uint64_t val = 1;
        ssize_t ret = ::write(efd, &val, sizeof(val));
        if (ret < 0) {
            LOG_ERROR("创建 eventfd 失败: %s", strerror(errno));
        }
    }
    void readEventFd(int efd) {
        uint64_t val;
        ssize_t ret = ::read(efd, &val, sizeof(val));
        if (ret < 0) {
            LOG_ERROR("创建 eventfd 失败: %s", strerror(errno));
        }
    }

    // 1. 初始化成员；  2. 进行额外的操作
    EventLoop::EventLoop()
        : _quit(false)
        , _callingPendingFunctors(false)
        , _poller(Poller::defaultPoller())
        , _threadId(::gettid()) 
        , _wakeupFd(createEventFd())
        , _wakeupChannel(new Channel(_wakeupFd, this)){
        // 将_wakeupChannel挂到poller上进行监控
        auto cb = std::bind(&EventLoop::handleRead, this);
        _wakeupChannel->setReadCallback(cb);
        _wakeupChannel->enableReading(); //启动weakupfd的读事件监控
    }
    EventLoop::~EventLoop(){
        ::close(_wakeupFd);
    }
    // 对channel进行事件监控操作的接口
    void EventLoop::updateChannel(Channel *channel){
        _poller->updateChannel(channel);
    }
    void EventLoop::removeChannel(Channel *channel){
        _poller->removeChannel(channel);
    }
    //事件循环的启动与结束
    // loop的死循环中干两件事情：
    //  1.  poller->wait()； 事件监控并处理
    //  2.  处理任务池中的任务
    void EventLoop::loop(){
                while(_quit == false) {
            std::vector<Channel*> actives;
            Timestamp now = _poller->wait(actives);
            for (auto channel : actives) {
                channel->handleEvent(now);
            }
            handlePendingFunctors();
        }
      } 
    void EventLoop::handlePendingFunctors() {
        //先获取所有要执行的任务
        _callingPendingFunctors = true;
        std::vector<Functor> functors;
        {
            std::unique_lock<std::mutex> lock(_mutex);
            functors.swap(_pendingFunctors);
        }
        // 执行任务
        for (auto &f : functors) {
            f();  /// queueInLoop（）这个函数内部可能会出现向任务池添加任务的场景
        }
        _callingPendingFunctors = false;
    }
    void EventLoop::quit(){
        // 1. 设置退出标志位
        _quit = true;
        // 2. 唤醒阻塞
        weakup();
    }
    // 任务池的操作：
    // 如果当前调用runInloop时候，本身就在loop所在的线程中，就直接执行，否则压入任务池
    void EventLoop::runInLoop(Functor functor){
        if (isInLoopThread()) {
            functor();
        }else {
            queueInLoop(std::move(functor));
        }
    } 
    // 将任务压入任务池
    void EventLoop::queueInLoop(Functor functor){
        {
            std::unique_lock<std::mutex> lock(_mutex);
            _pendingFunctors.emplace_back(std::move(functor));
        }
        // 没在loop线程中，或者正在进行任务池的任务处理
        if (isInLoopThread() == false || _callingPendingFunctors == true){
            weakup();
        }
    }
    //添加任务后唤醒可能存在的epoll_wait阻塞
    void EventLoop::weakup(){
        writeEventFd(_wakeupFd);
    } 
    void EventLoop::handleRead() {
        readEventFd(_wakeupFd);
    }

    //操作线程的判断//判断当前是否在loop线程中
    bool EventLoop::isInLoopThread(){
        return _threadId == ::gettid();
    } 
    //断言当前操作就在loop绑定的线程中
    void EventLoop::assertInLoopThread(){
        if (isInLoopThread() == false) {
            LOG_FATAL("assertInLoopThread Failed");
        }
    } 
}     