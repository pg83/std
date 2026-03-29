#include "thread.h"

#include "coro.h"
#include "runable.h"
#include "thread_iface.h"

#include <std/str/view.h>
#include <std/sys/throw.h>
#include <std/dbg/insist.h>
#include <std/str/builder.h>
#include <std/alg/exchange.h>

#include <unistd.h>
#include <pthread.h>

using namespace stl;

namespace {
    struct PosixThreadImpl: public ThreadIface {
        pthread_t thread;

        static void* threadFunc(void* arg) {
            return (((Runable*)arg)->run(), nullptr);
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

        void start(Runable& runable) override {
            if (pthread_create(&thread, nullptr, threadFunc, &runable)) {
                Errno().raise(StringBuilder() << StringView(u8"pthread_create failed"));
            }
        }
    };
}

Thread::Thread(Runable& runable)
    : impl(new PosixThreadImpl())
{
    impl->start(runable);
}

Thread::Thread(ThreadIface* iface)
    : impl(iface)
{
}

Thread::Thread(CoroExecutor* exec, Runable& runable)
    : impl(exec->createThread())
{
    impl->start(runable);
}

Thread::~Thread() noexcept {
    delete impl;
}

void Thread::join() noexcept {
    impl->join();
    delete exchange(impl, nullptr);
}

void Thread::detach() noexcept {
    impl->detach();
    delete exchange(impl, nullptr);
}

u64 Thread::threadId() const noexcept {
    return impl->threadId();
}

u64 Thread::currentThreadId() noexcept {
    return (u64)pthread_self();
}

void stl::detach(Runable& runable) {
    Thread(runable).detach();
}
