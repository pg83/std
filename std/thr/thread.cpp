#include "thread.h"

#include "coro.h"
#include "runable.h"

#include <std/str/view.h>
#include <std/sys/throw.h>
#include <std/dbg/insist.h>
#include <std/str/builder.h>
#include <std/mem/obj_pool.h>

#include <pthread.h>

using namespace stl;

namespace {
    struct PosixThreadImpl: public Thread {
        pthread_t thread_;

        static void* threadFunc(void* arg) {
            return (((Runable*)arg)->run(), nullptr);
        }

        void start(Runable& runable) override;
        void startWithStack(Runable& runable, void* stack, size_t stackSize);

        void join() noexcept override {
            STD_INSIST(pthread_join(thread_, nullptr) == 0);
        }

        u64 threadId() const noexcept override {
            static_assert(sizeof(pthread_t) <= sizeof(u64));
            return (u64)thread_;
        }
    };
}

void PosixThreadImpl::start(Runable& runable) {
    if (pthread_create(&thread_, nullptr, threadFunc, &runable)) {
        Errno().raise(StringBuilder() << StringView(u8"pthread_create failed"));
    }
}

void PosixThreadImpl::startWithStack(Runable& runable, void* stack, size_t stackSize) {
    pthread_attr_t attr;

    if (int rc = pthread_attr_init(&attr); rc) {
        Errno(rc).raise(StringBuilder() << StringView(u8"pthread_attr_init failed"));
    }

    if (int rc = pthread_attr_setstack(&attr, stack, stackSize); rc) {
        pthread_attr_destroy(&attr);
        Errno(rc).raise(StringBuilder() << StringView(u8"pthread_attr_setstack failed"));
    }

    int rc = pthread_create(&thread_, &attr, threadFunc, &runable);

    pthread_attr_destroy(&attr);

    if (rc) {
        Errno(rc).raise(StringBuilder() << StringView(u8"pthread_create failed"));
    }
}

u64 Thread::currentThreadId() noexcept {
    return (u64)pthread_self();
}

Thread* Thread::create(ObjPool* pool, Runable& runable) {
    auto t = pool->make<PosixThreadImpl>();

    t->start(runable);

    return t;
}

Thread* Thread::create(ObjPool* pool, CoroExecutor* exec, Runable& runable) {
    auto t = exec->createThread(pool);

    t->start(runable);

    return t;
}

Thread* Thread::create(ObjPool* pool, Runable& runable, void* stack, size_t stackSize) {
    auto t = pool->make<PosixThreadImpl>();

    t->startWithStack(runable, stack, stackSize);

    return t;
}
