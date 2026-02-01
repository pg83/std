#include "thread.h"

#include <std/mem/new.h>
#include <std/str/view.h>
#include <std/sys/throw.h>
#include <std/str/builder.h>

#include <errno.h>
#include <pthread.h>

using namespace Std;

struct Thread::Impl: public Newable {
    pthread_t thread;
    Runable* runable;

    explicit Impl(Runable& r)
        : runable(&r)
    {
        if (pthread_create(&thread, nullptr, thread_func, this)) {
            auto err = errno;

            throwErrno(err, StringBuilder() << StringView(u8"pthread_create failed"));
        }
    }

    static void* thread_func(void* arg) {
        ((Impl*)arg)->runable->run();
        return nullptr;
    }
};

Thread::Impl* Thread::impl() const noexcept {
    return (Impl*)storage_;
}

Thread::Thread(Runable& runable) {
    static_assert(sizeof(storage_) >= sizeof(Impl));

    new (storage_) Impl(runable);
}

Thread::~Thread() noexcept {
}

void Thread::join() {
    if (pthread_join(impl()->thread, nullptr)) {
        auto err = errno;

        throwErrno(err, StringBuilder() << StringView(u8"pthread_join failed"));
    }
}

void Thread::detach() {
    if (pthread_detach(impl()->thread)) {
        auto err = errno;

        throwErrno(err, StringBuilder() << StringView(u8"pthread_detach failed"));
    }
}

void Std::detach(Runable& runable) {
    Thread(runable).detach();
}
