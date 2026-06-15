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

namespace net {
    class Timestamp;
    using ReadCallback = std::function<void(Timestamp)>;
    using EventCallback = std::function<void()>;
    enum ChannelState {
        kNew = 1, //新建，还未添加事件监控
        kAdded, // 已添加事件监控
        kDeleted // 已解除事件监控
    };
    class Channel {
        public:
            int fd();
            uint32_t events();  //获取要监控的事件
            void setRevents(uint32_t e); //设置就绪事件
            void handleEvent(); //总的事件处理函数
        private:
            int _fd; 
            ChannelState _state; 
            uint32_t _events; //要监控的事件
            uint32_t _revents; // 保存实际就绪的事件
            ReadCallback _readCallback;
            EventCallback _writeCallback;
            EventCallback _errorCallback;
            EventCallback _closeCallback;
    };
}