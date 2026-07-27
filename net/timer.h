#pragma once
/*
    定时器的实现：
        TimerId：唯一标识并查找定时任务
        Timer：定时任务
        TimerQueue：定时任务管理器
*/

#include <stdint.h>
#include <atomic>
#include <functional>
#include <assert.h>
#include <set>
#include <unordered_set>
#include <memory>
#include "timestamp.h"

namespace net
{
    class Timer;
    class TimerQueue;
    class EventLoop;
    class Channel;

    class TimerId
    {
    public:
        TimerId(int64_t seq, Timer *timer)
            : _sequece(seq), _timer(timer) {}
        friend class TimerQueue;

    private:
        int64_t _sequece; // 序号
        Timer *_timer;    // 定时任务对象指针
    };

    class Timer
    {
    public:
        using Functor = std::function<void()>;

        Timer(Functor task, Timestamp when, double interval = 0)
            : _sequence(_numCreated.fetch_add(1)), _expired(when), _interval(interval), _repeated(interval > 0), _functor(std::move(task)) {}
        void run() { _functor(); }

        // 获取序号
        int64_t sequence() { return _sequence; }
        // 获取过期时间
        Timestamp expired() { return _expired; }
        // 获取间隔时间
        double interval() { return _interval; }
        // 是否是循环任务
        bool repeated() { return _repeated; }
        // 如果当前任务是一个循环任务则重置过期时间

        void restart(Timestamp now)
        {
            // 使用传入的当前系统时间 + 循环间隔时间 -》新的过期时间
            assert(_repeated);
            _expired = addTime(now, _interval);
        }

    private:
        static std::atomic<int64_t> _numCreated; // 累加器

        int64_t _sequence;  // 序号
        Timestamp _expired; // 任务过期时间
        double _interval;   // 循环间隔时间
        bool _repeated;     // 是否循环任务的标志位
        Functor _functor;   // 任务回调；
    };

    class TimerQueue
    {
        typedef std::pair<Timestamp, Timer *> Entry;

    public:
        // 初始化成员，函数内：设置timerChannel事件处理回调函数，启动事件监控
        TimerQueue(EventLoop *loop);
        // 关闭timerfd
        ~TimerQueue();
        // 添加定时任务: 打包了一个添加定时任务的操作，抛入到_loop任务池中
        TimerId addTimer(Timer::Functor task, Timestamp when, double interval);
        // 取消定时任务：打包一个取消操作，抛入到loop任务池中，避免产生线程安全问题
        void cancelTimer(TimerId tid);

    private:
        // 将定时任务，添加到_timers定时任务池中，重置定时器超时时间
        void addTimerInLoop(Timer *timer);
        void cancelTimerInLoop(TimerId tid);
        // 过期任务处理： 设置给_timerChannel读事件回调函数
        void handleRead();

        // 新增定时任务:1. 将任务添加到_timers中； 返回是否需要重置定时器时间
        bool insert(Timer *timer);
        // 获取所有的过期任务
        std::vector<Entry> getExpired(Timestamp now);
        // 针对处理完毕的过期任务进行重置：针对循环任务重新添加
        void resetExpired(std::vector<Entry> &tasks, Timestamp now);

    private:
        EventLoop *_loop; // 当前定时器对应的事件循环
        int _timerfd;     // 定时器描述符
        std::unique_ptr<Channel> _timerChannel;

        // 标志位：判断当前是否在过期任务处理中
        std::atomic<bool> _callingExpiredTimers;

        std::set<Entry> _timers;              // 定时任务池
        std::unordered_set<Timer *> _actives; // 为了快速查找一个定时任务是否存在
        std::unordered_set<Timer *> _cancels; // 取消池
    };
}