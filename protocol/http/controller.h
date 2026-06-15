#pragma once
#include "../protocol.h"

namespace proto{
namespace http {
    class HttpController : public Controller{
        public:
            void setStatus(int status);
        private:
    };
}
}