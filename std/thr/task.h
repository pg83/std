#pragma once

#include "runable.h"

#include <std/lib/node.h>

namespace stl {
    struct Task: public Runable, public IntrusiveNode {
    };

    template <typename V>
    struct TaskImpl: public Task {
        V v;

        TaskImpl(V vv)
            : v(vv)
        {
        }

        void run() override {
            v();
            delete this;
        }
    };

    template <typename T>
    auto makeTask(T t) {
        return new TaskImpl<T>(t);
    }
}
