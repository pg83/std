#include "mutex.h"

#include <std/str/view.h>
#include <std/sys/throw.h>
#include <std/dbg/insist.h>
#include <std/str/builder.h>

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
        Errno().raise(StringBuilder() << StringView(u8"pthread_mutex_init failed"));
    }
}

Mutex::~Mutex() noexcept {
    STD_INSIST(pthread_mutex_destroy(impl()) == 0);
}

void Mutex::lock() noexcept {
    STD_INSIST(pthread_mutex_lock(impl()) == 0);
}

void Mutex::unlock() noexcept {
    STD_INSIST(pthread_mutex_unlock(impl()) == 0);
}

bool Mutex::tryLock() noexcept {
    return pthread_mutex_trylock(impl()) == 0;
}
