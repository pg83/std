#pragma once

#include "runable.h"

#include <std/sys/types.h>

namespace stl {
    class ObjPool;

    struct Task;
    struct ThreadPool;
    struct EventIface;
    struct ThreadIface;
    struct CondVarIface;
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

    struct PollFD;

    struct CoroExecutor {
        virtual void join() noexcept = 0;
        virtual void yield() noexcept = 0;
        virtual u32 random() noexcept = 0;
        virtual Cont* me() const noexcept = 0;
        virtual Cont* spawnRun(SpawnParams params) = 0;
        virtual void parkWith(Runable&&, Task**) noexcept = 0;
        virtual u32 poll(int fd, u32 flags, u64 deadlineUs) = 0;
        virtual void offloadRun(ThreadPool* pool, Runable&& work) = 0;
        virtual size_t pollMulti(const PollFD* in, PollFD* out, size_t count, u64 deadlineUs) = 0;

        virtual int fsync(int fd) = 0;
        virtual int fdatasync(int fd) = 0;
        virtual ssize_t pread(int fd, void* buf, size_t len, off_t offset) = 0;
        virtual ssize_t pwrite(int fd, const void* buf, size_t len, off_t offset) = 0;

        virtual EventIface* createEvent() = 0;
        virtual ThreadIface* createThread() = 0;
        virtual CondVarIface* createCondVar() = 0;
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

        template <typename F>
        void offload(ThreadPool* pool, F f) {
            offloadRun(pool, makeRunable(f));
        }

        static CoroExecutor* create(ObjPool* pool, size_t threads);
        static CoroExecutor* create(ObjPool* pool, size_t threads, size_t reactors);
    };
}
