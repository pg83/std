#pragma once

#include "runable.h"

#include <std/sys/types.h>

namespace stl {
    class ObjPool;
    class StringView;

    struct Task;
    struct PollFD;
    struct PollGroup;
    struct DnsResult;
    struct ThreadPool;
    struct EventIface;
    struct VisitorFace;
    struct ThreadIface;
    struct CondVarIface;
    struct CoroExecutor;
    struct SemaphoreIface;

    struct Cont {
        u64 id() const noexcept;
    };

    struct CoroConfig {
        size_t threads;
        size_t reactors;
        size_t offloadThreads;
        size_t dnsResolvers;
        size_t maxDnsQueries;
        int dnsFamily;
        int dnsTimeout;
        int dnsTries;
        int dnsUdpMaxQueries;

        CoroConfig(size_t threads) noexcept;

        CoroConfig& setReactors(size_t v) noexcept;
        CoroConfig& setOffloadThreads(size_t v) noexcept;
        CoroConfig& setDnsResolvers(size_t v) noexcept;
        CoroConfig& setMaxDnsQueries(size_t v) noexcept;
        CoroConfig& setDnsFamily(int v) noexcept;
        CoroConfig& setDnsTimeout(int v) noexcept;
        CoroConfig& setDnsTries(int v) noexcept;
        CoroConfig& setDnsUdpMaxQueries(int v) noexcept;
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

    struct CoroExecutor {
        virtual void join() noexcept = 0;
        virtual void yield() noexcept = 0;
        virtual u32 random() noexcept = 0;
        virtual Cont* me() const noexcept = 0;
        virtual Cont* spawnRun(SpawnParams params) = 0;
        virtual void parkWith(Runable&&, Task**) noexcept = 0;
        virtual void offloadRun(ThreadPool* pool, Runable&& work) = 0;

        virtual u32 poll(PollFD pfd, u64 deadlineUs) = 0;
        virtual void poll(PollGroup* g, VisitorFace&& visitor, u64 deadlineUs) = 0;

        virtual DnsResult* resolve(ObjPool* pool, const StringView& name) = 0;

        virtual int fsync(int fd) = 0;
        virtual int fdatasync(int fd) = 0;
        virtual ssize_t pread(int fd, void* buf, size_t len, off_t offset) = 0;
        virtual ssize_t pwrite(int fd, const void* buf, size_t len, off_t offset) = 0;

        virtual void createEvent(void* buf) = 0;
        virtual ThreadIface* createThread() = 0;
        virtual CondVarIface* createCondVar() = 0;
        virtual SemaphoreIface* createSemaphore(size_t initial) = 0;

        void sleep();
        void sleep(u64 deadlineUs);
        void sleepTout(u64 timeoutUs);

        u32 poll(int fd, u32 flags);
        u32 poll(int fd, u32 flags, u64 deadlineUs);

        u64 currentCoroId() const noexcept;

        template <typename F>
        Cont* spawn(F f) {
            return spawnRun(SpawnParams().setRunable(f));
        }

        template <typename F>
        void offload(ThreadPool* pool, F f) {
            offloadRun(pool, makeRunable(f));
        }

        static CoroExecutor* create(ObjPool* pool, CoroConfig cfg);
        static CoroExecutor* create(ObjPool* pool, size_t threads);
        static CoroExecutor* create(ObjPool* pool, size_t threads, size_t reactors);
    };
}
