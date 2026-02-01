#include "mutex.h"
#include "cond_var.h"

#include <std/str/view.h>
#include <std/sys/throw.h>
#include <std/str/builder.h>

#include <errno.h>
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
        auto error = errno;

        throwErrno(error, StringBuilder() << StringView(u8"pthread_cond_init failed"));
    }
}

CondVar::~CondVar() noexcept {
    pthread_cond_destroy(impl());
}

void CondVar::wait(Mutex& mutex) noexcept {
    pthread_cond_wait(impl(), (pthread_mutex_t*)mutex.impl());
}

void CondVar::signal() noexcept {
    pthread_cond_signal(impl());
}

void CondVar::broadcast() noexcept {
    pthread_cond_broadcast(impl());
}
