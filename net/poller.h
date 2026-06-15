#pragma once
/*
    Poller模块：描述符事件监控模块
        1. 针对channel添加/修改监控
        2. 针对channel移除监控
        3. 这对所有描述符开始事件监控，并返回所有就绪的channel
*/
#include <vector>
#include <memory>
#include <unordered_map>

namespace net {
    class Channel;
    class Timestamp;
    class Poller;
    using PollerPtr = std::shared_ptr<Poller>;

    static int createEpoll();
    const static int EPOLL_TIMEOUT = 1000;
    const static int DETAULT_EVENT_SIZE = 16;
    static const int kNoneEvent = 0;
    static const int kReadEvent = EPOLLIN | EPOLLPRI;
    static const int kWriteEvent = EPOLLOUT;
    // muduo库中将描述符的监控+管理分开了
    //   解除事件监控：只epoll_ctl解除监控，但是不移_channels除管理
    //   移除监控管理：既解除监控又从_channels中移除管理
    class Poller {
        public:
            Poller() = default;
            virtual ~Poller() = default;
            // 对所有描述符进行监控，并获取就绪的描述符对应的channel
            virtual Timestamp wait(std::vector<Channel*> &actives) = 0;
            // 要实现新增 以及 修改事件
            virtual void updateChannel(Channel *channel) = 0;
            // 解除并移除监控，并移除管理
            virtual void removeChannel(Channel *channel) = 0;

             static PollerPtr defaultPoller();
        protected:
            std::unordered_map<int, Channel*> _channels;
    };

    class EpollPoller : public Poller {
        public:
            EpollPoller();
            virtual ~EpollPoller();
            virtual Timestamp wait(std::vector<Channel*> &actives) override;
            virtual void updateChannel(Channel *channel) override;
            virtual void removeChannel(Channel *channel) override;
        private:
            const char* eventStr(int op);
            void update(int op, Channel *channel);
        private:
            int _epfd;
            std::vector<struct epoll_event> _evs;
    };
}