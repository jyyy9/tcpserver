#include "timer.h"
#include "log.h"
#include <sys/time.h>
#include <cstring>
#include <errno.h>

namespace net
{
static int createTimerFd(){
    int fd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC); //::表示调用全局命名空间下的函数，防止与当前命名空间下的同名函数冲突
    // clock_realtime:系统实时时钟，系统启动后就开始计时，除非系统时间被修改，否则它不会被调整
    // clock_monotonic:单调时钟，系统启动后 就开始计时，不受系统时间修改的影响
    // tfd_nonblock:非阻塞模式
    // tfd_cloexec:当执行exec函数时，自动关闭该文件描述符
    return fd;
}
}

class Timer
{
public:
    using Functor = std::function<void()>;
    Timer(const Functor task, Timestamp when, double interval)
        : _sequence(_numCreated.fetch_add(1)), _expired(when), _interval(interval), _repeated(interval > 0), _functor(std::move(task)) {}
    void run() { _functor(); }
    int64_t sequence();                      // 获取定时器序号
    Timestamp expired() { return _expired; } // 获取定时器到期时间
    bool repeat() { return _repeated; }      // 获取定时器是否是循环任务的标志位
    void restart(Timestamp now)
    { // 重启定时器 
        assert(_repeated);
        _expired = addTime(now, _interval);
    }

private:
    static std::atomic<int64_t> _numCreated; // 累加器
    int64_t _sequence;                       // 定时器序号
    Timestamp _expired;                      // 定时器到期时间
    const TimerCallback _callback;           // 定时器回调函数
    double _interval;                        // 循环间隔时间
    bool _repeated;                          // 是否是循环任务的标志位
    Functor _functor;                        // 任务回调函数对象
};

    // 添加定时任务：打包了一个添加任务池操作，抛入到_loop任务池中
    TimerId TimerQueue::addTimer(Timer::Functor &task, Timestamp when, double interval){
        //1.创建一个定时任务对象

        //2.打包添加定时任务操作为一个仿函数对象，调用loop->runInLoop()方法将其抛入到loop任务池中
        auto cb=std::bind();
        _loop->runInLoop(cb);
        //3.根据定时任务对象，构造一个timerId对象并返回
        return TimerId(Timmer->sequence(),timmer);
    }
    void cancel(TimerId timerId);                                            // 取消定时任务
    void TimerQueue::addTimerInLoop(Timer *timer){
        //1.将定时任务对象添加到定时任务池中

        //2.判断是否需要重置定时器
    }
private:
    Eventloop *_loop;                       // 事件循环对象指针
    int _timerfd;                           // 定时器文件描述符
    std::unique_ptr<Channel> _timerChannel; // 定时器文件描述符通道对象指针

    // 标志位
    std::atomic<bool> _callingExpiredTimers; // 正在调用到期的定时  器的标志位

    using Entry = std::pair<Timestamp, Timer *>;
    std::set<Entry> _timers;              // 定时任务池集合
    std::unordered_map<Timer *> _cnacels; /// 取消池集合
    std::unordered_set<Timer *> _actives; /// 活跃池集合(为了快速查找一个定时任务的实现)（直接找到O（1））
};


void TimerQueue::cancelTimerInLoop(TimerId tid){
    assert(_actives.size==_timers.size());
    //1.如果任务再任务池中，直接移除
    if(_actives.find(tid.timer)!=_actives.end()){
        Entry entry={tid._timer};

    }
    //2.如果任务不在任务池中，并且当前正在过期任务处理中，将任务添加到取消池中



    assert(_actives.size==_timers.size());
}

//过期任务处理：设置给_timerChannel的读事件回调函数，触发时调用该函数
void TimerQueue::handleRead(){
    //1.从定时器描述符读取数据

    //2.获取所有的过期任务
    //3.对所有的过期任务进行处理
    //4.重置所有的过期任务
}

//获取所有的过期任务
std::vector<TimerQueue::Entry> TimerQueue::getExpired(Timestamp now){
    //1.构造一个Entry对象，表示过期任务的上界（给一个最大值）
    Entry entry={now,(Timer*)PTRDIFF_MAX};//ptrdiff_t是一个有符号整数类型，足够大以容纳任何对象的最大可能大小。将其作为Timer指针的值，表示一个非常大的地址，确保它在定时任务池中是最大的。

    //2.根据这个最大对象，获取到_timers中第一个大于等于该对象的迭代器（从begin到当前的任务都是过期任务）
    //3.将过期任务从_timers（定时任务池）中删除
    //4.返回这些过期任务
}

//针对处理完毕的过期任务，重置它们：针对循环任务，重置它们的过期时间，并重新添加到定时任务池中；针对非循环任务，直接删除它们
void TimerQueue::resetExpired(const std::vector<Entry> &tasks,Timestamp now){
    //1.遍历所有的过期任务，针对循环任务，判断它是否在取消池中，如果不在取消池中，重置它的过期时间，并重新添加到定时任务池中；针对非循环任务，直接删除它们
    //2.使用任务池最小节点重置定时器过期时间
    Timestamp expired=Timestamp::invalid();
    
}
