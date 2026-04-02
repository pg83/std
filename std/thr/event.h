#pragma once

#include "runable.h"

namespace stl {
    struct EventIface;
    struct CoroExecutor;

    class alignas(64) Event {
        char buf_[64];

        EventIface* impl() noexcept {
            return reinterpret_cast<EventIface*>(buf_);
        }

    public:
        Event();
        Event(CoroExecutor* exec);

        ~Event() noexcept;

        void signal() noexcept;
        void wait(Runable* cb) noexcept;

        void wait(Runable&& cb) noexcept {
            wait(&cb);
        }
    };
}
