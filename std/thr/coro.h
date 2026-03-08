#pragma once

#include <std/ptr/arc.h>
#include <std/ptr/intrusive.h>

namespace stl {
    class ThreadPool;
    struct CoroExecutor;

    struct Cont {
        virtual CoroExecutor* executor() noexcept = 0;
    };

    using coro = void(Cont*, void*);

    struct CoroExecutor: public ARC {
        using Ref = IntrusivePtr<CoroExecutor>;

        virtual ~CoroExecutor() noexcept;

        virtual void spawn(coro* fn, void* ctx) noexcept = 0;
        virtual void yield() noexcept = 0;

        static Ref create(size_t threads);
        static Ref create(ThreadPool* pool);
    };
}
