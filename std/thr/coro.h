#pragma once

#include "runable.h"

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

        virtual Cont* me() = 0;
        virtual void yield() noexcept = 0;
        virtual void spawnCoro(coro* fn, void* ctx) = 0;

        void spawnRun(Runable* runable);

        template <typename F>
        void spawn(F f) {
            spawnRun(makeRunable([this, f]() {
                f(this->me());
            }));
        }

        static Ref create(size_t threads);
        static Ref create(ThreadPool* pool);
    };
}
