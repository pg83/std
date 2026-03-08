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

    struct SpawnParams {
        size_t stackSize;
        void* stackPtr;
        Runable* runable;

        SpawnParams() noexcept;

        SpawnParams& setStackSize(size_t v) noexcept {
            stackSize = v;
            return *this;
        }

        SpawnParams& setStackPtr(void* v) noexcept {
            stackPtr = v;
            return *this;
        }

        SpawnParams& setRunablePtr(Runable* v) noexcept {
            runable = v;
            return *this;
        }

        template <typename F>
        SpawnParams& setRunable(F f) {
            return setRunablePtr(makeRunablePtr(f));
        }
    };

    struct CoroExecutor: public ARC {
        using Ref = IntrusivePtr<CoroExecutor>;

        virtual ~CoroExecutor() noexcept;

        virtual void yield() noexcept = 0;
        virtual Cont* me() const noexcept = 0;
        virtual void spawnRun(SpawnParams params) = 0;
        virtual ThreadPool* pool() const noexcept = 0;

        template <typename F>
        void spawn(F f) {
            spawnRun(SpawnParams().setRunable([this, f]() {
                f(this->me());
            }));
        }

        static Ref create(size_t threads);
        static Ref create(ThreadPool* pool);
    };
}
