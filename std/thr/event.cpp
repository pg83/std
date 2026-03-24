#include "event.h"
#include "coro.h"
#include "event_iface.h"

#include <std/sys/atomic.h>

#include <unistd.h>
#include <limits.h>
#include <sys/syscall.h>
#include <linux/futex.h>

using namespace stl;

namespace {
    struct FutexEventImpl: public EventIface {
        u32 state_;

        FutexEventImpl() noexcept
            : state_(0)
        {
        }

        void wait(Runable&& cb) noexcept override {
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
    };
}

Event::Event()
    : impl_(new FutexEventImpl())
{
}

Event::Event(CoroExecutor* exec)
    : impl_(exec->createEvent())
{
}

Event::Event(EventIface* iface) noexcept
    : impl_(iface)
{
}

Event::~Event() noexcept {
    delete impl_;
}

void Event::wait(Runable&& cb) noexcept {
    impl_->wait(static_cast<Runable&&>(cb));
}

void Event::signal() noexcept {
    impl_->signal();
}
