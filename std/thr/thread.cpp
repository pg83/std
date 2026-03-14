#include "thread.h"
#include "coro.h"
#include "mutex.h"
#include "runable.h"
#include "thread_iface.h"

#include <std/str/view.h>
#include <std/sys/throw.h>
#include <std/ptr/scoped.h>
#include <std/dbg/insist.h>
#include <std/str/builder.h>
#include <std/alg/exchange.h>

#include <unistd.h>
#include <pthread.h>

using namespace stl;

namespace {
    struct PosixThreadImpl: public ThreadIface {
        pthread_t thread;
        Runable* runable;

        explicit PosixThreadImpl(Runable& r)
            : runable(&r)
        {
        }

        static void* threadFunc(void* arg) {
            return (((PosixThreadImpl*)arg)->runable->run(), nullptr);
        }

        void join() noexcept override {
            STD_INSIST(pthread_join(thread, nullptr) == 0);
            delete this;
        }

        void detach() noexcept override {
            STD_INSIST(pthread_detach(thread) == 0);
            delete this;
        }

        u64 threadId() const noexcept override {
            static_assert(sizeof(pthread_t) <= sizeof(u64));
            return (u64)thread;
        }

        void start() override {
            if (pthread_create(&thread, nullptr, threadFunc, this)) {
                Errno().raise(StringBuilder() << StringView(u8"pthread_create failed"));
            }
        }
    };
}

Thread::Thread(Runable& runable)
    : Thread(new PosixThreadImpl(runable))
{
}

Thread::Thread(ThreadIface* iface)
    : impl(iface)
{
    impl->start();
}

Thread::Thread(CoroExecutor* exec, Runable& runable)
    : Thread(exec->createThread(runable))
{
}

Thread::~Thread() noexcept {
    delete impl;
}

void Thread::join() noexcept {
    exchange(impl, nullptr)->join();
}

void Thread::detach() noexcept {
    exchange(impl, nullptr)->detach();
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
        Mutex m;
        Thread thr;

        Helper(Runable* r) noexcept
            : slave(r)
            , m(true)
            , thr(*this)
        {
        }

        void run() override {
            ScopedPtr<Helper> that(this);
            // race in musl libc
            m.lock();
            slave->run();
            thr.detach();
        }
    };

    (new Helper(&runable))->m.unlock();
}
