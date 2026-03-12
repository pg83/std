#pragma once

#include "runable.h"

#include <std/lib/node.h>
#include <std/sys/types.h>

namespace stl {
    struct Task: public Runable, public IntrusiveNode {
        virtual u8 priority() const noexcept;
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
