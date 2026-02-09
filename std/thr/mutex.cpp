#include "mutex.h"

#include <std/str/view.h>
#include <std/sys/throw.h>
#include <std/dbg/insist.h>
#include <std/str/builder.h>

#include <pthread.h>

using namespace Std;

struct Mutex::Impl: public pthread_mutex_t {
    inline Impl() {
        if (pthread_mutex_init(this, nullptr) != 0) {
            Errno().raise(StringBuilder() << StringView(u8"pthread_mutex_init failed"));
        }
    }

    inline ~Impl() noexcept {
        STD_INSIST(pthread_mutex_destroy(this) == 0);
    }
};

Mutex::Mutex()
    : impl(new Impl())
{
}

Mutex::~Mutex() noexcept {
    delete impl;
}

void Mutex::lock() noexcept {
    STD_INSIST(pthread_mutex_lock(impl) == 0);
}

void Mutex::unlock() noexcept {
    STD_INSIST(pthread_mutex_unlock(impl) == 0);
}

bool Mutex::tryLock() noexcept {
    return pthread_mutex_trylock(impl) == 0;
}
