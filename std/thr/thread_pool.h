#pragma once

#include "thread.h"

#include <std/ptr/arc.h>
#include <std/lib/node.h>
#include <std/ptr/intrusive.h>

namespace Std {
    struct Task: public Runable, public IntrusiveNode {
    };

    struct ThreadPool: public ARC {
        virtual ~ThreadPool() noexcept;

        virtual void submit(Task& task) noexcept = 0;
        virtual void join() noexcept = 0;

        using Ref = IntrusivePtr<ThreadPool>;

        static Ref simple(size_t threads);
        static Ref workStealing(size_t threads);
    };
}
