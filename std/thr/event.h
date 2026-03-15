#pragma once

namespace stl {
    struct CoroExecutor;

    class Event {
        struct Impl;
        Impl* impl;

    public:
        Event();
        Event(CoroExecutor* exec);

        ~Event() noexcept;

        void set() noexcept;
        void wait() noexcept;
    };
}
