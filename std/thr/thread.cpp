#include "thread.h"
#include "runable.h"

#include <std/mem/new.h>
#include <std/str/view.h>
#include <std/sys/throw.h>
#include <std/ptr/scoped.h>
#include <std/dbg/insist.h>
#include <std/str/builder.h>
#include <std/alg/destruct.h>

#include <errno.h>
#include <pthread.h>

using namespace Std;

struct Thread::Impl: public Newable {
    pthread_t thread;
    Runable* runable;

    explicit Impl(Runable& r)
        : runable(&r)
    {
        if (pthread_create(&thread, nullptr, threadFunc, this)) {
            auto err = errno;

            throwErrno(err, StringBuilder() << StringView(u8"pthread_create failed"));
        }
    }

    static void* threadFunc(void* arg) {
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
    destruct(impl());
}

void Thread::join() noexcept {
    STD_INSIST(pthread_join(impl()->thread, nullptr) == 0);
}

void Thread::detach() noexcept {
    STD_INSIST(pthread_detach(impl()->thread) == 0);
}

void Std::detach(Runable& runable) {
    struct Helper: public Runable {
        Runable* slave;
        ScopedPtr<Thread> thr;

        inline Helper(Runable* r) noexcept
            : slave(r)
            , thr(nullptr)
        {
        }

        void start() {
            (thr.ptr = new Thread(*this))->detach();
        }

        void run() noexcept override {
            ScopedPtr<Helper> that(this);
            slave->run();
        }
    };

    ScopedPtr<Helper> guard(new Helper(&runable));

    guard.ptr->start();
    guard.drop();
}
