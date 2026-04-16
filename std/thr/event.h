#pragma once

#include "runable.h"

namespace stl {
    struct CoroExecutor;

    struct Event {
        struct alignas(64) Buf {
            char data[64];
        };

        virtual ~Event() noexcept;

        virtual void signal() noexcept = 0;
        virtual void wait(Runable& cb) noexcept = 0;

        void wait(Runable&& cb) noexcept {
            wait(cb);
        }

        static Event* create(Buf& buf);
        static Event* create(Buf& buf, CoroExecutor* exec);
    };
}
