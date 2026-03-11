#pragma once

#include "runable.h"

#include <std/ptr/arc.h>
#include <std/sys/types.h>
#include <std/ptr/intrusive.h>

namespace stl {
    struct ThreadPool;
    struct MutexIface;
    struct ThreadIface;
    struct CondVarIface;
    struct ChannelIface;
    struct CoroExecutor;

    struct Cont {
        u32 poll(int fd, u32 flags);

        virtual CoroExecutor* executor() noexcept = 0;
        virtual u32 poll(int fd, u32 flags, u64 timeoutUs) = 0;
    };

    struct SpawnParams {
        size_t stackSize;
        void* stackPtr;
        Runable* runable;

        SpawnParams() noexcept;

        SpawnParams& setStackPtr(void* v) noexcept;
        SpawnParams& setStackSize(size_t v) noexcept;
        SpawnParams& setRunablePtr(Runable* v) noexcept;

        template <typename F>
        SpawnParams& setRunable(F f) {
            return setRunablePtr(makeRunablePtr(f));
        }
    };

    struct CoroExecutor: public ARC {
        using Ref = IntrusivePtr<CoroExecutor>;

        virtual ~CoroExecutor() noexcept;

        virtual void yield() noexcept = 0;
        virtual u32 random() noexcept = 0;
        virtual Cont* me() const noexcept = 0;
        virtual MutexIface* createMutex() = 0;
        virtual CondVarIface* createCondVar() = 0;
        virtual void spawnRun(SpawnParams params) = 0;
        virtual ThreadPool* pool() const noexcept = 0;
        virtual ChannelIface* createChannel(size_t cap) = 0;
        virtual ThreadIface* createThread(Runable& runable) = 0;

        template <typename F>
        void spawn(F f) {
            spawnRun(SpawnParams().setRunable([this, f]() {
                f(this->me());
            }));
        }

        static Ref create(size_t threads);
        static Ref create(size_t threads, size_t reactors);
        static Ref create(ThreadPool* pool);
        static Ref create(ThreadPool* pool, size_t reactors);
    };
}
