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
    struct PosixCondVarImpl: public CondVar, public pthread_cond_t {
        PosixCondVarImpl() {
            if (pthread_cond_init(this, nullptr) != 0) {
                Errno().raise(StringBuilder() << StringView(u8"pthread_cond_init failed"));
            }
        }

        ~PosixCondVarImpl() noexcept {
            STD_INSIST(pthread_cond_destroy(this) == 0);
        }

        void wait(Mutex& mutex) noexcept override {
            STD_INSIST(pthread_cond_wait(this, (pthread_mutex_t*)mutex.nativeHandle()) == 0);
        }

        void signal() noexcept override {
            STD_INSIST(pthread_cond_signal(this) == 0);
        }

        void broadcast() noexcept override {
            STD_INSIST(pthread_cond_broadcast(this) == 0);
        }
    };
}

CondVar* CondVar::create(ObjPool* pool) {
    return pool->make<PosixCondVarImpl>();
}
