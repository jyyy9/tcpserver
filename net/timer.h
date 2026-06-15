#pragma once
/*
    定时器的实现：
    TimerId：定时器ID
    Timer：定时器
    TimerQueue：定时器队列
*/

#include <stdint.h>
#include <atomic>
#include <functional>
#include <assert.h>
#include <map>
#include <unordered_map>
#include "timestamp.h"

namespace net
{
    class TimerId
    {
    public:
        TimerId(int64_t seq, Timer *timer);
        friend class TimerQueue;

    private:
        int64_t _sequence; // 序号
        Timer *_timer;     // 定时任务对象指针
    };

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

    class TimerQueue
    {
    public:
        TimerQueue(Eventloop *loop);
        ~TimerQueue();
        //添加定时任务：打包了一个添加任务池操作，抛入到_loop任务池中
        TimerId addTimer(Timer::Functor &task, Timestamp when, double interval);//addtimer有可能会跨线程操作
        void cancel(TimerId timerId); //取消定时任务
    private:
        Eventloop *_loop;                       // 事件循环对象指针
        int _timerfd;                           // 定时器文件描述符
        std::unique_ptr<Channel> _timerChannel; // 定时器文件描述符通道对象指针

        // 标志位
        std::atomic<bool> _callingExpiredTimers; // 正在调用到期的定时器的标志位

        using Entry = std::pair<Timestamp, Timer *>;
        std::set<Entry> _timers;              // 定时任务池集合
        std::unordered_map<Timer *> _cnacels; /// 取消池集合
        std::unordered_set<Timer *> _actives; /// 活跃池集合(为了快速查找一个定时任务的实现)（直接找到O（1））
    };

}