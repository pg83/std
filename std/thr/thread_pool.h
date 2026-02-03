#pragma once

#include <std/ptr/arc.h>
#include <std/ptr/intrusive.h>

namespace Std {
    struct Task;

    struct ThreadPool: public ARC {
        virtual ~ThreadPool() noexcept;

        virtual void submit(Task& task) noexcept = 0;
        virtual void join() noexcept = 0;

        using Ref = IntrusivePtr<ThreadPool>;

        static Ref simple(size_t threads);
        static Ref workStealing(size_t threads);
    };
}
