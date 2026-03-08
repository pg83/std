#include "thread.h"
#include "runable.h"

#include <std/str/view.h>
#include <std/sys/throw.h>
#include <std/ptr/scoped.h>
#include <std/dbg/insist.h>
#include <std/str/builder.h>

#include <pthread.h>

using namespace stl;

struct Thread::Impl {
    pthread_t thread;
    Runable* runable;

    explicit Impl(Runable& r)
        : runable(&r)
    {
        if (pthread_create(&thread, nullptr, threadFunc, this)) {
            Errno().raise(StringBuilder() << StringView(u8"pthread_create failed"));
        }
    }

    static void* threadFunc(void* arg) {
        return (((Impl*)arg)->runable->run(), nullptr);
    }
};

Thread::Thread(Runable& runable)
    : impl(new Impl(runable))
{
}

Thread::~Thread() {
    delete impl;
}

void Thread::join() {
    STD_INSIST(pthread_join(impl->thread, nullptr) == 0);
}

void Thread::detach() {
    STD_INSIST(pthread_detach(impl->thread) == 0);
}

u64 Thread::threadId() const {
    static_assert(sizeof(pthread_t) <= sizeof(u64));
    return (u64)impl->thread;
}

u64 Thread::currentThreadId() {
    return (u64)pthread_self();
}

void stl::detach(Runable& runable) {
    struct Helper: public Runable {
        Runable* slave;
        ScopedPtr<Thread> thr;

        Helper(Runable* r)
            : slave(r)
            , thr(nullptr)
        {
        }

        void start() {
            (thr.ptr = new Thread(*this))->detach();
        }

        void run() override {
            ScopedPtr<Helper> that(this);
            slave->run();
        }
    };

    ScopedPtr<Helper> guard(new Helper(&runable));

    guard.ptr->start();
    guard.drop();
}
