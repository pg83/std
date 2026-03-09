#include "mutex.h"
#include "cond_var.h"
#include "coro.h"
#include "mutex_iface.h"
#include "cond_var_iface.h"

#include <std/str/view.h>
#include <std/sys/throw.h>
#include <std/dbg/insist.h>
#include <std/str/builder.h>

#include <pthread.h>

using namespace stl;

struct PosixCondVarImpl: public CondVarIface, public pthread_cond_t {
    PosixCondVarImpl() {
        if (pthread_cond_init(this, nullptr) != 0) {
            Errno().raise(StringBuilder() << StringView(u8"pthread_cond_init failed"));
        }
    }

    ~PosixCondVarImpl() noexcept override {
        STD_INSIST(pthread_cond_destroy(this) == 0);
    }

    void wait(MutexIface* mutex) noexcept override {
        STD_INSIST(pthread_cond_wait(this, (pthread_mutex_t*)mutex->nativeHandle()) == 0);
    }

    void signal() noexcept override {
        STD_INSIST(pthread_cond_signal(this) == 0);
    }

    void broadcast() noexcept override {
        STD_INSIST(pthread_cond_broadcast(this) == 0);
    }
};

CondVar::CondVar()
    : CondVar(new PosixCondVarImpl())
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
    delete impl;
}

void CondVar::wait(Mutex& mutex) noexcept {
    impl->wait(mutex.impl);
}

void CondVar::signal() noexcept {
    impl->signal();
}

void CondVar::broadcast() noexcept {
    impl->broadcast();
}
