#include "coro.h"
#include "mutex.h"
#include "cond_var.h"

#include <std/str/view.h>
#include <std/sys/throw.h>
#include <std/dbg/insist.h>
#include <std/str/builder.h>
#include <std/mem/obj_pool.h>

#include <pthread.h>

using namespace stl;

namespace {
    // pthread_cond_t cannot be a base class: it is a union on glibc
    struct PosixCondVarImpl: public CondVar {
        pthread_cond_t cond;

        PosixCondVarImpl() {
            if (pthread_cond_init(&cond, nullptr) != 0) {
                Errno().raise(StringBuilder() << StringView(u8"pthread_cond_init failed"));
            }
        }

        ~PosixCondVarImpl() noexcept {
            STD_INSIST(pthread_cond_destroy(&cond) == 0);
        }

        void wait(Mutex* mutex) noexcept override {
            STD_INSIST(pthread_cond_wait(&cond, (pthread_mutex_t*)mutex->nativeHandle()) == 0);
        }

        void signal() noexcept override {
            STD_INSIST(pthread_cond_signal(&cond) == 0);
        }

        void broadcast() noexcept override {
            STD_INSIST(pthread_cond_broadcast(&cond) == 0);
        }
    };
}

CondVar* CondVar::create(ObjPool* pool) {
    return pool->make<PosixCondVarImpl>();
}

CondVar* CondVar::create(ObjPool* pool, CoroExecutor* exec) {
    return exec->createCondVar(pool);
}
