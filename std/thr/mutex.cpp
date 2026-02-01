#include "mutex.h"

#include <std/str/view.h>
#include <std/sys/throw.h>
#include <std/str/builder.h>

#include <errno.h>
#include <pthread.h>

using namespace Std;

namespace {
    static inline pthread_mutex_t* mutex(char* buf) noexcept {
        return reinterpret_cast<pthread_mutex_t*>(buf);
    }
}

Mutex::Mutex() {
    static_assert(sizeof(storage_) >= sizeof(pthread_mutex_t));

    if (pthread_mutex_init(mutex(storage_), nullptr) != 0) {
        auto error = errno;

        throwErrno(error, StringBuilder() << StringView(u8"pthread_mutex_init failed"));
    }
}

Mutex::~Mutex() noexcept {
    pthread_mutex_destroy(mutex(storage_));
}

void Mutex::lock() noexcept {
    pthread_mutex_lock(mutex(storage_));
}

void Mutex::unlock() noexcept {
    pthread_mutex_unlock(mutex(storage_));
}

bool Mutex::try_lock() noexcept {
    return pthread_mutex_trylock(mutex(storage_)) == 0;
}
