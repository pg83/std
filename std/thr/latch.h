#pragma once

namespace stl {
    class Latch {
        struct Impl;
        Impl* impl;

    public:
        explicit Latch(int n);
        ~Latch() noexcept;

        void arrive() noexcept;
        void wait() noexcept;
    };
}
