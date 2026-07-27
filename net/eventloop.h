#pragma once
/*
    eventloop核心功能：
      1. channel事件监控
      2. 任务池解决连接线程安全问题
      3. 定时任务操作
*/
#include <memory>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <functional>
#include "poller.h"
#include "timer.h"

namespace net
{
    class Channel;
    static int createEventFd();
    static void writeEventFd(int efd);
    static void readEventFd(int efd);
    //
    class EventLoop
    {
    public:
        using Functor = std::function<void()>;
        EventLoop();
        ~EventLoop();
        // 对channel进行事件监控操作的接口
        void updateChannel(Channel *channel);
        void removeChannel(Channel *channel);
        // 事件循环的启动与结束
        //  loop的死循环中干两件事情：
        //   1.  poller->wait()； 事件监控并处理
        //   2.  处理任务池中的任务
        void loop(); // 内部while(_quit) { poller->wait() }
        void quit();
        // 任务池的操作：
        // 如果当前调用runInloop时候，本身就在loop所在的线程中，就直接执行，否则压入任务池
        void runInLoop(Functor functor);
        // 将任务压入任务池
        void queueInLoop(Functor functor);
        void weakup(); // 添加任务后唤醒可能存在的epoll_wait阻塞

        // 操作线程的判断
        bool isInLoopThread();     // 判断当前是否在loop线程中
        void assertInLoopThread(); // 断言当前操作就在loop绑定的线程中

        //定时任务的操作: 定点执行, 延迟任务, 循环任务，取消任务
            TimerId runAt(Timestamp when, Timer::Functor functor);
            TimerId runAfter(double delaySec, Timer::Functor functor);
            TimerId runEvery(double interval, Timer::Functor functor);
            void cancel(TimerId tid);
    private:
        // weakupfd的可读事件处理回调函数
        void handleRead();
        // 执行处理任务池的任务
        void handlePendingFunctors();

    private:
        std::atomic<bool> _quit;         // 循环退出标志
        bool _callingPendingFunctors;    // 是否正处于任务池的任务处理中
        std::unique_ptr<Poller> _poller; // 事件监控对象
        pid_t _threadId;                 // 当前eventloop对象绑定的线程ID --标识每个loop对象所在的线程

        int _wakeupFd; // 事件监控唤醒描述符 挂在epoll上专门用于唤醒epoll_wait
        std::unique_ptr<Channel> _wakeupChannel;

 std::unique_ptr<TimerQueue> _timerQueue;

        std::mutex _mutex;
        std::vector<Functor> _pendingFunctors; // 任务池

        // TimerQueue _timerQueue;// 定时器
    };
    
    /*
        EventLoopThread类：将EventLoop对象与线程绑定到一起
    */
    class EventLoopThread {
        public:
            EventLoopThread();
            ~EventLoopThread();
            // 获取事件循环的时候，必须保证循环对象构造完毕才能返回
            EventLoop* startLoop();
        private:
            void threadFunc(); //线程的入口函数  latch
        private:
            std::mutex _mutex;
            std::condition_variable _ctx;
            EventLoop *_loop; //事件循环成员,必须在线程内部构造
            std::thread _thread; //线程对象
    };
    /*
        池化事件循环线程
    */
    class EventLoopThreadPool {
        public:
            EventLoopThreadPool(EventLoop* baseloop, int threadCount = 0);
            ~EventLoopThreadPool();
            // 设置事件循环数量
            void setLoopThreadNum(int count); 
            // 根据事件循环数量，构造EventLoopThread，添加到数组中
            void start();
            // 获取负载均衡后的派发事件循环对象指针，若池化数量为0，则返回主事件循环
            EventLoop* nextLoop();
        private:
            int _looptThreadNum; //事件循环线程数量
            int _nextIdx; //RR轮转下标(当前采用RR轮转负载均衡策略)
            EventLoop* _baseloop;
            std::vector<std::unique_ptr<EventLoopThread>> _loopThreads;
            std::vector<EventLoop*> _loops;
    };
}