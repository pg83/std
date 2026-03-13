#pragma once

#include "task.h"
#include "runable.h"

#include <std/ptr/arc.h>
#include <std/sys/types.h>
#include <std/ptr/intrusive.h>

namespace stl {
    u64 registerTlsKey() noexcept;

    class PCG32;

    struct ThreadPool: public ARC {
        virtual ~ThreadPool() noexcept;

        virtual void join() noexcept = 0;
        virtual void beforeBlock() noexcept;
        virtual PCG32& random() noexcept = 0;
        virtual void** tls(u64 key) noexcept = 0;
        virtual void submitTask(Task* task) noexcept = 0;

        template <typename F>
        void submit(F f) {
            submitTask(makeTask(f));
        }

        using Ref = IntrusivePtr<ThreadPool>;

        static Ref sync();
        static Ref simple(size_t threads);
        static Ref workStealing(size_t threads);
    };
}
