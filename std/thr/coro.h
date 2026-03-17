#pragma once

#include "runable.h"

#include <std/ptr/arc.h>
#include <std/sys/types.h>
#include <std/ptr/intrusive.h>

namespace stl {
    class ObjPool;

    struct ThreadPool;
    struct MutexIface;
    struct ThreadIface;
    struct CondVarIface;
    struct ChannelIface;
    struct CoroExecutor;

    struct Cont {
        u64 id() const noexcept;
    };

    struct SpawnParams {
        size_t stackSize;
        void* stackPtr;
        Runable* runable;
        u8 priority;
        bool system;

        SpawnParams() noexcept;

        SpawnParams& setPriority(u8 v) noexcept;
        SpawnParams& setSystem(bool v) noexcept;
        SpawnParams& setStackPtr(void* v) noexcept;
        SpawnParams& setStackSize(size_t v) noexcept;
        SpawnParams& setRunablePtr(Runable* v) noexcept;
        SpawnParams& setStack(void* v, size_t len) noexcept;
        SpawnParams& setStack(ObjPool* v, size_t len) noexcept;

        template <typename F>
        SpawnParams& setRunable(F f) {
            return setRunablePtr(makeRunablePtr(f));
        }
    };

    struct CoroExecutor: public ARC {
        using Ref = IntrusivePtr<CoroExecutor>;

        virtual ~CoroExecutor() noexcept;

        virtual void join() noexcept = 0;
        virtual void yield() noexcept = 0;
        virtual u32 random() noexcept = 0;
        virtual Cont* me() const noexcept = 0;
        virtual MutexIface* createMutex() = 0;
        virtual CondVarIface* createCondVar() = 0;
        virtual Cont* spawnRun(SpawnParams params) = 0;
        virtual ChannelIface* createChannel(size_t cap) = 0;
        virtual u32 poll(int fd, u32 flags, u64 deadlineUs) = 0;
        virtual ThreadIface* createThread(Runable& runable) = 0;

        u32 poll(int fd, u32 flags);
        u64 currentCoroId() const noexcept;

        template <typename F>
        Cont* spawn(F f) {
            return spawnRun(SpawnParams().setRunable(f));
        }

        static Ref create(size_t threads);
        static Ref create(size_t threads, size_t reactors);
    };
}
