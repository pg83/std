#include "mutex.h"
#include "coro.h"
#include "cond_var.h"
#include "cond_var_iface.h"

#include <std/str/view.h>
#include <std/sys/throw.h>
#include <std/dbg/insist.h>
#include <std/str/builder.h>

#include <std/mem/obj_pool.h>

#include <pthread.h>

using namespace stl;

namespace {
    struct PosixCondVarImpl: public CondVarIface, public pthread_cond_t {
        PosixCondVarImpl() {
            if (pthread_cond_init(this, nullptr) != 0) {
                Errno().raise(StringBuilder() << StringView(u8"pthread_cond_init failed"));
            }
        }

        ~PosixCondVarImpl() noexcept override {
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

CondVarIface* CondVar::createDefault() {
    return new PosixCondVarImpl();
}

CondVarIface* CondVar::createDefault(ObjPool* pool) {
    struct Impl: public PosixCondVarImpl {
        bool owned() const noexcept override {
            return true;
        }
    };

    return pool->make<Impl>();
}

CondVar* CondVar::create(ObjPool* pool) {
    return pool->make<CondVar>(createDefault(pool));
}

CondVar::CondVar()
    : CondVar(createDefault())
{
}

CondVar::CondVar(CondVarIface* iface)
    : impl(iface)
{
}

CondVar::CondVar(CoroExecutor* exec)
    : CondVar(exec->createCondVar())
{
}

CondVar::~CondVar() noexcept {
    if (!impl->owned()) {
        delete impl;
    }
}

void CondVar::wait(Mutex& mutex) noexcept {
    impl->wait(mutex);
}

void CondVar::signal() noexcept {
    impl->signal();
}

void CondVar::broadcast() noexcept {
    impl->broadcast();
}
