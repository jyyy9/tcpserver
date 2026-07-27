#include "timer.h"
#include "log.h"
#include "channel.h"
#include "eventloop.h"
#include <cstring>
#include <errno.h>
#include <unistd.h>
#include <sys/timerfd.h>

namespace net {
    std::atomic<int64_t> Timer::_numCreated = 0;
    static int createTimerFd() {
        int fd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
        if (fd < 0) {
            LOG_FATAL("创建定时器失败: %s", strerror(errno));
        }
        return fd;
    }
    struct timespec howMuchFromNow(Timestamp when) {
        Timestamp now = Timestamp::now();
        //微秒级的时间差值
        int64_t detal = when.microSecondsSinceEpoch() - now.microSecondsSinceEpoch();
        if (detal < 100) {
            detal = 100;
        }
        struct timespec retval;
        retval.tv_sec = detal / Timestamp::kMicroSecondPerSecond;
        retval.tv_nsec = (detal % Timestamp::kMicroSecondPerSecond) * 1000;
        return retval;
    }
    static void resetTimerFd(int timerfd, Timestamp when) {
        // int timerfd_settime(int fd, int flags, const struct itimerspec *new_value, old_value)
        // 从当前系统时间到when的差值： when - now = detal 
        struct itimerspec newval;
        struct itimerspec oldval;
        memset(&newval, 0x00, sizeof(newval));
        memset(&oldval, 0x00, sizeof(oldval));
        newval.it_value = howMuchFromNow(when);
        int ret = timerfd_settime(timerfd, 0, &newval, &oldval);
        if (ret < 0) {
            LOG_ERROR("设置定时器过期时间失败: %s", strerror(errno));
        }
    }
    void readTimerFd(int timerfd) {
        int64_t val;
        ssize_t ret = ::read(timerfd, &val, sizeof(val));
        if (ret < 0) {
            LOG_ERROR("读取定时器超时时间数据失败: %s", strerror(errno));
        }
    }


    // 初始化成员，函数内：设置timerChannel事件处理回调函数，启动事件监控
    TimerQueue::TimerQueue(EventLoop *loop)
        : _loop(loop)
        , _timerfd(createTimerFd())
        , _timerChannel(new Channel(_timerfd, loop))
        , _callingExpiredTimers(false) {
        auto cb = std::bind(&TimerQueue::handleRead, this);
        _timerChannel->setReadCallback(cb);
        _timerChannel->enableReading(); //启动读事件监控
    }
    // 关闭timerfd
    TimerQueue::~TimerQueue(){
        //1. 解除并移除timerfd的事件监控
        _timerChannel->disableAll();
        _timerChannel->remove();
        //2. 关闭timerfd描述符
        ::close(_timerfd);
    }
    //添加定时任务: 打包了一个添加定时任务的操作，抛入到_loop任务池中
    TimerId TimerQueue::addTimer(Timer::Functor task, Timestamp when, double interval){
        // 1. 构造定时任务对象
        Timer *timer = new Timer(std::move(task), when, interval);
        // 2. 打包添加定时任务操作为一个仿函数对象，调用loop->runInLoop
        auto cb = std::bind(&TimerQueue::addTimerInLoop, this, timer);
        _loop->runInLoop(cb);
        // 3. 根据定时任务对象，构造一个TimerId对象，并返回
        return TimerId(timer->sequence(), timer);
    }
    //取消定时任务：打包一个取消操作，抛入到loop任务池中，避免产生线程安全问题
    void TimerQueue::cancelTimer(TimerId tid){
        auto cb = std::bind(&TimerQueue::cancelTimerInLoop, this, tid);
        _loop->runInLoop(cb);
    }



    //将定时任务，添加到_timers定时任务池中，重置定时器超时时间
    void TimerQueue::addTimerInLoop(Timer* timer){
        // 1. 将定时任务添加到定时任务池中
        bool changed = insert(timer);
        // 2. 判断是否需要重置定时器超时事件，若需要，则重置
        if (changed) {
            // 获取最小节点的过期时间
            Timestamp when = _timers.begin()->first;
            resetTimerFd(_timerfd, when);
        }
    }

    // 新增定时任务:1. 将任务添加到_timers中； 返回是否需要重置定时器时间
    bool TimerQueue::insert(Timer* timer){
        assert(_actives.size() == _timers.size());
        bool changed = false;
        // 任务池为空，或者当前任务的过期时间，小于任务池中最小节点的过期时间，则需要重置定时器
        if (_timers.empty() || timer->expired() < _timers.begin()->first) {
            changed = true;
        }
        // 将任务添加到_timers中
        // 1. 根据timer构造Entry对象
        Entry entry = {timer->expired(), timer};
        // 2. 添加entry到timers中， 并添加timer到actives中
        auto ret1 = _timers.insert(entry);
        assert(ret1.second);
        auto ret2 = _actives.insert(timer);
        assert(ret2.second);
        assert(_actives.size() == _timers.size());
        // 返回是否需要重置定时器时间
        return changed;
    } 

    void TimerQueue::cancelTimerInLoop(TimerId tid){
        assert(_actives.size() == _timers.size());
        if (_actives.find(tid._timer) != _actives.end()) {
            // 1. 如果任务在任务池中，则直接移除
            Entry entry = {tid._timer->expired(), tid._timer};
            _timers.erase(entry);
            _actives.erase(tid._timer);
        }else {
            // 2. 如果任务不在任务池中，并且当前正在过期任务处理中，则将当前任务添加到取消池
            if (_callingExpiredTimers == true) {
                _cancels.insert(tid._timer);
            }
        }
        assert(_actives.size() == _timers.size());
    }
    // 过期任务处理： 设置给_timerChannel读事件回调函数
    void TimerQueue::handleRead(){
        // 1. 从定时器描述符读取数据
        readTimerFd(_timerfd);
        // 2. 获取所有的过期任务
        Timestamp now = Timestamp::now();
        auto expired_tasks = getExpired(now);
        // 3. 对所有过期任务进行处理（执行内部的回调函数）
        _callingExpiredTimers = true;
        _cancels.clear(); //唯一的作用就是为了防止循环任务在自己的回调函数中取消自己
        for (auto& task : expired_tasks) {
            task.second->run();
        }
        _callingExpiredTimers = false;
        // 4. 重置处理所有的过期任务（针对循环任务重新添加到任务池）
        resetExpired(expired_tasks, now);
    } 

    // 获取所有的过期任务
    std::vector<TimerQueue::Entry> TimerQueue::getExpired(Timestamp now){
        assert(_actives.size() == _timers.size());
        // 1. 构造一个指定时间的最大Entry对象
        Entry entry = {now, (Timer*)PTRDIFF_MAX};
        // 2. 根据这个最大对象，获取到_timers中的第一个大于等于该对象的迭代器（从begin到当前位置都是过期任务）
        auto end = _timers.lower_bound(entry);
        auto start = _timers.begin();
        std::vector<TimerQueue::Entry> retval;
        std::copy(start, end, std::back_inserter(retval));
        // 3. 从定时任务池中，将这些任务移除
        _timers.erase(start, end);
        for (auto &it : retval) {
            _actives.erase(it.second);
        }
        assert(_actives.size() == _timers.size());
        // 4. 返回这些过期任务
        return retval;
    }
    // 针对处理完毕的过期任务进行重置：针对循环任务重新添加
    void TimerQueue::resetExpired(std::vector<Entry> &tasks, Timestamp now){
        // 1. 遍历所有的过期任务，针对循环任务，判断是否在取消池中，如果不在，则重置过期时间，添加到任务池
        for (auto &it : tasks) {
            Timer* timer = it.second;
            if (timer->repeated() && _cancels.find(timer) == _cancels.end()) {
                timer->restart(now);
                insert(timer);
            }else {
                delete timer;
            }
        }
        // 2. 使用任务池最小节点重置定时器的过期时间
        Timestamp expired = Timestamp::invalid(); //获取一个无效的时间
        if (_timers.empty() == false) {
            expired = _timers.begin()->first;
        }
        if (expired.valid()) {
            resetTimerFd(_timerfd, expired);
        }
    }
}