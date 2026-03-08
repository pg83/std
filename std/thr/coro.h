#pragma once

#include "pool.h"

#include <std/ptr/arc.h>
#include <std/ptr/intrusive.h>

namespace stl {
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

        static Ref create(ThreadPool::Ref pool);
    };
}
