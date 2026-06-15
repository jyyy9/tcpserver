#pragma once
/*
    Poller模块：描述符事件监控模块
        1. 针对channel添加/修改监控
        2. 针对channel移除监控
        3. 这对所有描述符开始事件监控，并返回所有就绪的channel
*/
#include <vector>
#include <unordered_map>

namespace net {
    class Channel;
    class Timestamp;
    // muduo库中将描述符的监控+管理分开了
    //   解除事件监控：只epoll_ctl解除监控，但是不移_channels除管理
    //   移除监控管理：既解除监控又从_channels中移除管理
    class Poller {
        public:
            virtual Timestamp wait(std::vector<Channel*> &actives) = 0;
            // 要实现新增 以及 修改事件
            virtual void updateChannel(Channel *channel) = 0;
            // 解除并移除监控
            virtual void removeChannel(Channel *channel) = 0;
        private:
            std::unordered_map<int, Channel*> _channels;
    };
}