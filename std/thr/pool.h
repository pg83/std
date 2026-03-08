#pragma once

#include <std/ptr/arc.h>
#include <std/sys/types.h>
#include <std/ptr/intrusive.h>

namespace stl {
    struct Task;
    struct Runable;

    u64 registerTlsKey();

    struct ThreadPool: public ARC {
        virtual ~ThreadPool();

        virtual void submitTask(Task& task) = 0;
        virtual void join() = 0;
        virtual void** tls(u64 key) = 0;

        void submit(Runable& runable);

        using Ref = IntrusivePtr<ThreadPool>;

        static Ref sync();
        static Ref simple(size_t threads);
        static Ref workStealing(size_t threads);
    };
}
