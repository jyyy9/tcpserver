#pragma once
/*
    抽象事件处理器模块：对描述符进行事件处理描述管理
    设计：
       1. 要监控的描述符
       2. 要监控的事件
       3. 描述符实际就绪的事件
       4. 针对事件如何处理的回调函数
*/
#include <stddef.h>
#include <iostream>
#include <functional>
#include <stdint.h>

namespace net
{
    class Timestamp;
    class EventLoop;
    using ReadCallback = std::function<void(Timestamp)>;
    using EventCallback = std::function<void()>;
    enum ChannelState
    {
        kNew = 1, // 新建，还未添加事件监控
        kAdded,   // 已添加事件监控
        kDeleted  // 已解除事件监控
    };
    class Channel
    {
    public:
        Channel(int fd, EventLoop *loop);
        ~Channel();
        int fd() { return _fd; }
        ChannelState state() { return _state; }
        void setState(ChannelState state) { _state = state; }
        uint32_t events() { return _events; }
        void setRevents(uint32_t e) { _revents = e; }
        void setTie(std::shared_ptr<void> &obj)
        {
            _tied = true;
            _tie = obj;
        }
        void setReadCallback(ReadCallback cb) { _readCallback = std::move(cb); }
        void setWriteCallback(EventCallback cb) { _writeCallback = std::move(cb); }
        void setErrorCallback(EventCallback cb) { _errorCallback = std::move(cb); }
        void setCloseCallback(EventCallback cb) { _closeCallback = std::move(cb); }
        // 判断当前描述符是否处于无事件/写/读监控状态
        bool isNoneEvent() const { return _events == kNoneEvent; }
        bool isWriting() const { return _events == kWriteEvent; }
        bool isReading() const { return _events == kReadEvent; }
        // 对当前描述符进行读事件操作
        void enableReading()
        {
            // 1. 将当前描述符要监控的事件设置为读事件
            _events |= kReadEvent;
            // 2. 找到当前描述符分配的Poller，调用它的updateChannel(this)
            update();
        }
        void enableWriting()
        {
            _events |= kWriteEvent;
            update();
        }
        void disableReading()
        {
            _events &= ~kReadEvent;
            update();
        }
        void disableWriting()
        {
            _events &= ~kWriteEvent;
            update();
        }
        void disableAll()
        {
            _events = kNoneEvent;
            update();
        }
        // 总的事件处理函数
        void handleEvent(Timestamp recvTime)
        {
            if (_tied)
            {
                if (_tie.lock())
                { // 能否获取到外部对象
                    handleEventWithGuard(recvTime);
                }
            }
            else
            {
                // 如果不需要关心外部的管理对象生命周期，则直接调用
                handleEventWithGuard(recvTime);
            }
        }
        void remove(); // 解除监控，并移除管理
    private:
        void update(); // 用于实际的调用poller描述符事件操作 {  poller->updateChannel(this) }
        void handleEventWithGuard(Timestamp recvTime)
        {
            _eventHandling = true;
            // 在这个函数中，根据实际就绪的事件，调用不同的回调函数进行事件处理
            // 就绪了连接挂断事件，且当前没有新数据到来，直接调用关闭回调
            if (_revents & EPOLLHUP && !(_revents & kReadEvent))
            {
                if (_closeCallback)
                    _closeCallback();
            }
            if (_revents & kReadEvent)
            {
                if (_readCallback)
                    _readCallback(recvTime);
            }
            if (_revents & kWriteEvent)
            {
                if (_writeCallback)
                    _writeCallback();
            }
            if (_revents & EPOLLERR)
            {
                if (_errorCallback)
                    _errorCallback();
            }
            _eventHandling = false;
        }

    private:
        EventLoop *_loop;
        int _fd;
        ChannelState _state;
        uint32_t _events;  // 要监控的事件
        uint32_t _revents; // 保存实际就绪的事件

        bool _tied;
        // 观察者模式的一种特殊使用方式，通过它观察外部管理对象是否还存在
        std::weak_ptr<void> _tie;

        bool _addedToLoop;   // 标志位：判断channel是否在poller监控中
        bool _eventHandling; // 标志位：当前channel是否正在事件处理中

        ReadCallback _readCallback;
        EventCallback _writeCallback;
        EventCallback _errorCallback;
    };
}