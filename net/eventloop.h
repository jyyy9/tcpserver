#pragma once
#include <memory>

namespace net {
    class Channel;
    class Poller;
    class EventLoop {
        public:
            void updateChannel(Channel *channel);
            void removeChannel(Channel *channel);
        private:
            std::unique_ptr<Poller> _poller;
    };
}