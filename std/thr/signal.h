#pragma once

namespace stl {
    struct SignalIface;
    struct CoroExecutor;

    class Signal {
        SignalIface* impl_;

    public:
        Signal();
        Signal(CoroExecutor* exec);
        ~Signal() noexcept;

        void set() noexcept;
        void wait() noexcept;
    };
}
