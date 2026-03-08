#pragma once

#include "runable.h"

#include <std/ptr/arc.h>
#include <std/sys/types.h>
#include <std/ptr/intrusive.h>

namespace stl {
    struct Task;

    u64 registerTlsKey() noexcept;

    struct ThreadPool: public ARC {
        virtual ~ThreadPool() noexcept;

        virtual void submitTask(Task& task) noexcept = 0;
        virtual void join() noexcept = 0;
        virtual void** tls(u64 key) noexcept = 0;

        void submitRun(Runable& runable);

        void submitTmp(Runable&& runable) {
            submitRun(runable);
        }

        template <typename F>
        void submit(F f) {
            submitTmp(makeRunable(f));
        }

        using Ref = IntrusivePtr<ThreadPool>;

        static Ref sync();
        static Ref simple(size_t threads);
        static Ref workStealing(size_t threads);
    };
}
