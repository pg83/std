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

        void start(Runable& runable) override {
            if (pthread_create(&thread_, nullptr, threadFunc, &runable)) {
                Errno().raise(StringBuilder() << StringView(u8"pthread_create failed"));
            }
        }

        void join() noexcept override {
            STD_INSIST(pthread_join(thread_, nullptr) == 0);
        }

        u64 threadId() const noexcept override {
            static_assert(sizeof(pthread_t) <= sizeof(u64));
            return (u64)thread_;
        }
    };
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
