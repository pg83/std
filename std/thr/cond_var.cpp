#include "mutex.h"
#include "cond_var.h"

#include <std/str/view.h>
#include <std/sys/throw.h>
#include <std/dbg/insist.h>
#include <std/str/builder.h>

#include <pthread.h>

using namespace Std;

struct CondVar::Impl: public pthread_cond_t {
    inline Impl() {
        if (pthread_cond_init(this, nullptr) != 0) {
            Errno().raise(StringBuilder() << StringView(u8"pthread_cond_init failed"));
        }
    }

    inline ~Impl() noexcept {
        STD_INSIST(pthread_cond_destroy(this) == 0);
    }
};

CondVar::CondVar()
    : impl(new Impl())
{
}

CondVar::~CondVar() noexcept {
    delete impl;
}

void CondVar::wait(Mutex& mutex) noexcept {
    STD_INSIST(pthread_cond_wait(impl, (pthread_mutex_t*)mutex.impl()) == 0);
}

void CondVar::signal() noexcept {
    STD_INSIST(pthread_cond_signal(impl) == 0);
}

void CondVar::broadcast() noexcept {
    STD_INSIST(pthread_cond_broadcast(impl) == 0);
}
