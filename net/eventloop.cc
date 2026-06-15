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
       , _wakeupChannel(new Channel(_wakeupFd, this))
        , _timerQueue(new TimerQueue(this)){
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
        TimerId EventLoop::runAt(Timestamp when, Timer::Functor functor) {
        return _timerQueue->addTimer(std::move(functor), when, 0);
    }
    TimerId EventLoop::runAfter(double delaySec, Timer::Functor functor){
        // 2. 获取系统时间戳,根据延迟时间计算得到定点时间戳
        Timestamp when = addTime(Timestamp::now(), delaySec);
        // 3. 添加定时任务
        return _timerQueue->addTimer(std::move(functor), when, 0);
    }
    TimerId EventLoop::runEvery(double interval, Timer::Functor functor){
        // 2. 获取系统时间戳,根据延迟时间计算得到定点时间戳
        Timestamp when = addTime(Timestamp::now(), interval);
        // 3. 添加定时任务
        return _timerQueue->addTimer(std::move(functor), when, interval);
    }
    void EventLoop::cancel(TimerId tid) {
        _timerQueue->cancelTimer(tid);
    }


    EventLoopThread::EventLoopThread()
        : _loop(nullptr)
        , _thread(&EventLoopThread::threadFunc, this) {}
    // 等待线程退出
    EventLoopThread::~EventLoopThread() {
        _loop->quit(); //退出事件循环
        _thread.join();
        _loop = nullptr;
    }
    // 获取事件循环的时候，必须保证循环对象构造完毕才能返回
    EventLoop* EventLoopThread::startLoop() {
        std::unique_lock<std::mutex> lock(_mutex);
        _ctx.wait(lock, [this](){
            return _loop != NULL; //不等于空则返回，等于空则循环等待
        });
        return _loop;
    }
    //线程的入口函数  latch
    void EventLoopThread::threadFunc() {
        EventLoop loop; //实例化一个局部变量
        {
            std::unique_lock<std::mutex> lock(_mutex);
            _loop = &loop;
            _ctx.notify_all();
        }
        loop.loop(); //让事件循环跑起来
    }


    EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseloop, int threadCount)
        : _looptThreadNum(threadCount)
        , _nextIdx(0)
        , _baseloop(baseloop) {}

    EventLoopThreadPool::~EventLoopThreadPool() {
        for (auto &loop : _loops) {
            loop->quit();
        }
    }
    // 设置事件循环数量
    void EventLoopThreadPool::setLoopThreadNum(int count) {
        _looptThreadNum = count;
    }
    // 获取负载均衡后的派发事件循环对象指针，若池化数量为0，则返回主事件循环
    EventLoop* EventLoopThreadPool::nextLoop() {
        // 根据_nextIdx，获取数组_loops元素； 获取完毕后对_nextIdx重置
        EventLoop* loop = _loops[_nextIdx++];
        if (_nextIdx >= _loops.size()) {
            _nextIdx = 0;
        }
        return loop;
    }
    // 根据事件循环数量，构造EventLoopThread，添加到数组中
    void EventLoopThreadPool::start() {
        for (int i = 0; i < _looptThreadNum; ++i) {
            EventLoopThread* loopthread = new EventLoopThread;
            EventLoop* loop = loopthread->startLoop();
            _loopThreads.push_back(std::unique_ptr<EventLoopThread>(loopthread));
            _loops.push_back(loop);
        }
    }
}     