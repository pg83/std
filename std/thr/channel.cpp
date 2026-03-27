#include "channel.h"
#include "event.h"
#include "mutex.h"
#include "guard.h"

#include <std/sys/crt.h>
#include <std/lib/list.h>
#include <std/dbg/insist.h>
#include <std/lib/ring_buf.h>

using namespace stl;

struct Channel::Impl {
    struct Waiter: public IntrusiveNode {
        Event* ev;
        void* value;
        bool valueSet;
    };

    CoroExecutor* exec_;
    Mutex mu_;
    IntrusiveList senders_;
    IntrusiveList receivers_;
    bool closed_;

    Impl() noexcept
        : exec_(nullptr)
        , closed_(false)
    {
    }

    Impl(CoroExecutor* exec) noexcept
        : exec_(exec)
        , mu_(exec)
        , closed_(false)
    {
    }

    bool sendOne(void* v) noexcept {
        if (auto w = (Waiter*)receivers_.popFrontOrNull(); w) {
            w->value = v;
            w->valueSet = true;
            w->ev->signal();

            return true;
        }

        return bufferOne(v);
    }

    bool recvOne(void** out) noexcept {
        if (unbufferOne(out)) {
            return true;
        }

        if (auto w = (Waiter*)senders_.popFrontOrNull(); w) {
            *out = w->value;
            w->ev->signal();

            return true;
        }

        return false;
    }

    virtual bool bufferOne(void*) noexcept {
        return false;
    }

    virtual bool unbufferOne(void**) noexcept {
        return false;
    }

    void enqueue(void* v) noexcept {
        LockGuard guard(mu_);

        STD_INSIST(!closed_);

        if (sendOne(v)) {
            return;
        }

        Event ev(exec_);
        Waiter w;

        w.ev = &ev;
        w.value = v;
        w.valueSet = true;

        senders_.pushBack(&w);
        ev.wait(makeRunable([&] {
            guard.drop();
            mu_.unlock();
        }));
    }

    bool dequeue(void** out) noexcept {
        LockGuard guard(mu_);

        if (recvOne(out)) {
            return true;
        }

        if (closed_) {
            return false;
        }

        Event ev(exec_);
        Waiter w;

        w.ev = &ev;
        w.value = nullptr;
        w.valueSet = false;

        receivers_.pushBack(&w);

        ev.wait(makeRunable([&] {
            guard.drop();
            mu_.unlock();
        }));

        if (w.valueSet) {
            *out = w.value;

            return true;
        }

        return false;
    }

    bool tryEnqueue(void* v) noexcept {
        LockGuard guard(mu_);

        STD_INSIST(!closed_);

        return sendOne(v);
    }

    bool tryDequeue(void** out) noexcept {
        LockGuard guard(mu_);

        return recvOne(out);
    }

    void close() noexcept {
        LockGuard guard(mu_);

        STD_INSIST(!closed_);
        STD_INSIST(senders_.empty());

        closed_ = true;

        while (auto w = (Waiter*)receivers_.popFrontOrNull()) {
            w->ev->signal();
        }
    }

    virtual ~Impl() noexcept {
    }
};

namespace {
    struct BufferedImpl: public Channel::Impl {
        RingBuffer buf_;

        BufferedImpl(size_t capacity) noexcept
            : buf_((void**)(this + 1), capacity)
        {
        }

        BufferedImpl(CoroExecutor* exec, size_t capacity) noexcept
            : Impl(exec)
            , buf_((void**)(this + 1), capacity)
        {
        }

        void* operator new(size_t, void* p) noexcept {
            return p;
        }

        void operator delete(void* p) noexcept {
            freeMemory(p);
        }

        bool bufferOne(void* v) noexcept override {
            if (!buf_.full()) {
                buf_.push(v);

                return true;
            }

            return false;
        }

        bool unbufferOne(void** out) noexcept override {
            if (!buf_.empty()) {
                *out = buf_.pop();

                if (auto w = (Waiter*)senders_.popFrontOrNull(); w) {
                    buf_.push(w->value);
                    w->ev->signal();
                }

                return true;
            }

            return false;
        }
    };
}

Channel::Channel()
    : impl_(new Impl())
{
}

Channel::Channel(size_t cap)
    : impl_(cap == 0
                ? new Impl()
                : new (allocateMemory(sizeof(BufferedImpl) + cap * sizeof(void*))) BufferedImpl(cap))
{
}

Channel::Channel(CoroExecutor* exec)
    : Channel(exec, 0)
{
}

Channel::Channel(CoroExecutor* exec, size_t cap)
    : impl_(cap == 0
                ? new Impl(exec)
                : new (allocateMemory(sizeof(BufferedImpl) + cap * sizeof(void*))) BufferedImpl(exec, cap))
{
}

Channel::~Channel() noexcept {
    delete impl_;
}

void Channel::enqueue(void* v) noexcept {
    impl_->enqueue(v);
}

bool Channel::dequeue(void** out) noexcept {
    return impl_->dequeue(out);
}

bool Channel::tryEnqueue(void* v) noexcept {
    return impl_->tryEnqueue(v);
}

bool Channel::tryDequeue(void** out) noexcept {
    return impl_->tryDequeue(out);
}

void Channel::close() noexcept {
    impl_->close();
}
