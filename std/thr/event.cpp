#include "event.h"
#include "coro.h"
#include "guard.h"
#include "mutex.h"
#include "cond_var.h"
#include "event_iface.h"

#include <std/mem/new.h>
#include <std/sys/atomic.h>

#ifdef __linux__
    #include <unistd.h>
    #include <limits.h>
    #include <sys/syscall.h>
    #include <linux/futex.h>
#endif

using namespace stl;

namespace {
#ifdef __linux__
    struct alignas(64) FutexEventImpl: public EventIface, public Newable {
        u32 state_;

        FutexEventImpl() noexcept
            : state_(0)
        {
        }

        void wait(Runable& cb) noexcept override {
            u32 cur = stdAtomicFetch(&state_, MemoryOrder::Acquire);
            cb.run();

            while (cur == 0) {
                syscall(SYS_futex, &state_, FUTEX_WAIT, 0, nullptr, nullptr, 0);
                cur = stdAtomicFetch(&state_, MemoryOrder::Acquire);
            }
        }

        void signal() noexcept override {
            stdAtomicStore(&state_, (u32)1, MemoryOrder::Release);
            syscall(SYS_futex, &state_, FUTEX_WAKE, 1, nullptr, nullptr, 0);
        }

        static void operator delete(void*) noexcept {
        }
    };

    using DefaultEventImpl = FutexEventImpl;
#else
    struct PosixEventImpl: public EventIface, public Newable {
        Mutex mu_;
        CondVar cv_;
        bool signaled_;

        PosixEventImpl() noexcept
            : signaled_(false)
        {
        }

        void wait(Runable& cb) noexcept override {
            mu_.lock();
            cb.run();

            while (!signaled_) {
                cv_.wait(mu_);
            }

            mu_.unlock();
        }

        void signal() noexcept override {
            LockGuard guard(mu_);
            signaled_ = true;
            cv_.signal();
        }

        static void operator delete(void*) noexcept {
        }
    };

    using DefaultEventImpl = PosixEventImpl;
#endif
}

Event::Event() {
    new (buf_) DefaultEventImpl();
}

Event::Event(CoroExecutor* exec) {
    if (exec) {
        exec->createEvent(buf_);
    } else {
        new (buf_) DefaultEventImpl();
    }
}

Event::~Event() noexcept {
    delete impl();
}

void Event::wait(Runable* cb) noexcept {
    impl()->wait(*cb);
}

void Event::signal() noexcept {
    impl()->signal();
}
