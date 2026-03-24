#pragma once

#include "runable.h"

namespace stl {
    struct EventIface;
    struct CoroExecutor;

    class Event {
        EventIface* impl_;

    public:
        Event();
        Event(CoroExecutor* exec);
        Event(EventIface* iface) noexcept;

        ~Event() noexcept;

        void signal() noexcept;
        void wait(Runable* cb) noexcept;

        void wait(Runable&& cb) noexcept {
            wait(&cb);
        }
    };
}
