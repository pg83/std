#pragma once

#include "runable.h"

#include <std/ptr/arc.h>
#include <std/sys/types.h>
#include <std/ptr/intrusive.h>

#include <sys/types.h>

namespace stl {
    class ObjPool;

    struct ThreadPool;
    struct ThreadIface;
    struct CondVarIface;
    struct ChannelIface;
    struct CoroExecutor;
    struct SemaphoreIface;

    struct Cont {
        u64 id() const noexcept;
    };

    struct SpawnParams {
        size_t stackSize;
        void* stackPtr;
        Runable* runable;

        SpawnParams() noexcept;

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
        virtual Cont* spawnRun(SpawnParams params) = 0;
        virtual u32 poll(int fd, u32 flags, u64 deadlineUs) = 0;
        virtual ssize_t pread(int fd, void* buf, size_t len, off_t offset) = 0;
        virtual ssize_t pwrite(int fd, const void* buf, size_t len, off_t offset) = 0;

        virtual CondVarIface* createCondVar() = 0;
        virtual ChannelIface* createChannel(size_t cap) = 0;
        virtual ThreadIface* createThread(Runable& runable) = 0;
        virtual SemaphoreIface* createSemaphore(size_t initial) = 0;

        void sleep();
        void sleep(u64 deadlineUs);
        void sleepTout(u64 timeoutUs);

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
