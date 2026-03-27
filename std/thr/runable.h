#pragma once

namespace stl {
    struct Runable {
        virtual void run() = 0;
    };

    template <typename V, bool del>
    struct RunableImpl final: public Runable {
        V v;

        RunableImpl(V vv)
            : v(vv)
        {
        }

        void run() override {
            v();

            if constexpr (del) {
                delete this;
            }
        }
    };

    template <typename T>
    auto makeRunable(T t) {
        return RunableImpl<T, false>(t);
    }

    template <typename T>
    auto makeRunablePtr(T t) {
        return new RunableImpl<T, true>(t);
    }
}
