#pragma once

namespace stl {
    class Barrier {
        struct Impl;
        Impl* impl;

    public:
        explicit Barrier(int n);
        ~Barrier();

        void wait();
    };
}
