#pragma once

#include <std/sys/types.h>

namespace stl {
    class WaitGroup {
        struct Impl;
        Impl* impl;

    public:
        WaitGroup();
        ~WaitGroup() noexcept;

        void add(size_t n) noexcept;
        void done() noexcept;
        void wait() noexcept;
    };
}
