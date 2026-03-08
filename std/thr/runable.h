#pragma once

namespace stl {
    struct Runable {
        virtual void run() = 0;
    };

    template <typename V>
    struct RunableImpl: public Runable {
        V v;

        RunableImpl(V vv)
            : v(vv)
        {
        }

        void run() override {
            v();
            delete this;
        }
    };

    template <typename T>
    auto makeRunable(T t) {
        return new RunableImpl<T>(t);
    }
}
