#include "mutex.h"
#include "cond_var.h"

#include <std/str/view.h>
#include <std/sys/throw.h>
#include <std/dbg/insist.h>
#include <std/str/builder.h>

#include <pthread.h>

using namespace Std;

struct CondVar::Impl: public pthread_cond_t {
};

CondVar::Impl* CondVar::impl() noexcept {
    return (Impl*)storage_;
}

CondVar::CondVar() {
    static_assert(sizeof(storage_) >= sizeof(pthread_cond_t));

    if (pthread_cond_init(impl(), nullptr) != 0) {
        Errno().raise(StringBuilder() << StringView(u8"pthread_cond_init failed"));
    }
}

CondVar::~CondVar() noexcept {
    STD_INSIST(pthread_cond_destroy(impl()) == 0);
}

void CondVar::wait(Mutex& mutex) noexcept {
    STD_INSIST(pthread_cond_wait(impl(), (pthread_mutex_t*)mutex.impl()) == 0);
}

void CondVar::signal() noexcept {
    STD_INSIST(pthread_cond_signal(impl()) == 0);
}

void CondVar::broadcast() noexcept {
    STD_INSIST(pthread_cond_broadcast(impl()) == 0);
}
