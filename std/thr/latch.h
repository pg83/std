#pragma once

#include <std/sys/types.h>

namespace stl {
    class Latch {
        struct Impl;
        Impl* impl;

    public:
        explicit Latch(size_t n);
        ~Latch() noexcept;

        void arrive() noexcept;
        void wait() noexcept;
    };
}
