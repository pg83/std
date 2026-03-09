#include "thread.h"
#include "thread_iface.h"
#include "runable.h"
#include "coro.h"

#include <std/str/view.h>
#include <std/sys/throw.h>
#include <std/ptr/scoped.h>
#include <std/dbg/insist.h>
#include <std/str/builder.h>

#include <pthread.h>

using namespace stl;

namespace {
    struct PosixThreadImpl: public ThreadIface {
        pthread_t thread;
        Runable* runable;

        explicit PosixThreadImpl(Runable& r)
            : runable(&r)
        {
            if (pthread_create(&thread, nullptr, threadFunc, this)) {
                Errno().raise(StringBuilder() << StringView(u8"pthread_create failed"));
            }
        }

        static void* threadFunc(void* arg) {
            return (((PosixThreadImpl*)arg)->runable->run(), nullptr);
        }

        void join() noexcept override {
            STD_INSIST(pthread_join(thread, nullptr) == 0);
        }

        void detach() noexcept override {
            STD_INSIST(pthread_detach(thread) == 0);
        }

        u64 threadId() const noexcept override {
            static_assert(sizeof(pthread_t) <= sizeof(u64));
            return (u64)thread;
        }
    };
}

Thread::Thread(Runable& runable)
    : impl(new PosixThreadImpl(runable))
{
}

Thread::Thread(ThreadIface* iface)
    : impl(iface)
{
}

Thread::Thread(CoroExecutor* exec, Runable& runable)
    : impl(exec->createThread(runable))
{
}

Thread::~Thread() noexcept {
    delete impl;
}

void Thread::join() noexcept {
    impl->join();
}

void Thread::detach() noexcept {
    impl->detach();
}

u64 Thread::threadId() const noexcept {
    return impl->threadId();
}

u64 Thread::currentThreadId() noexcept {
    return (u64)pthread_self();
}

void stl::detach(Runable& runable) {
    struct Helper: public Runable {
        Runable* slave;
        ScopedPtr<Thread> thr;

        Helper(Runable* r) noexcept
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
