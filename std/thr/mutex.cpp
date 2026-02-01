#include "mutex.h"

#include <std/str/view.h>
#include <std/sys/throw.h>
#include <std/str/builder.h>

#include <errno.h>
#include <pthread.h>

using namespace Std;

struct Mutex::Impl: public pthread_mutex_t {
};

Mutex::Impl* Mutex::impl() noexcept {
    return reinterpret_cast<Impl*>(storage_);
}

Mutex::Mutex() {
    static_assert(sizeof(storage_) >= sizeof(pthread_mutex_t));

    if (pthread_mutex_init(impl(), nullptr) != 0) {
        auto error = errno;

        throwErrno(error, StringBuilder() << StringView(u8"pthread_mutex_init failed"));
    }
}

Mutex::~Mutex() noexcept {
    pthread_mutex_destroy(impl());
}

void Mutex::lock() noexcept {
    pthread_mutex_lock(impl());
}

void Mutex::unlock() noexcept {
    pthread_mutex_unlock(impl());
}

bool Mutex::tryLock() noexcept {
    return pthread_mutex_trylock(impl()) == 0;
}
