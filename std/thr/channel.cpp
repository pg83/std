#include "channel.h"
#include "event.h"
#include "mutex.h"
#include "guard.h"

#include <std/sys/crt.h>
#include <std/lib/list.h>
#include <std/dbg/insist.h>
#include <std/alg/bits.h>

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
        , mu_(Mutex::spinLock(exec))
        , closed_(false)
    {
    }

    bool sendOne(void* v) noexcept;
    bool recvOne(void** out) noexcept;

    virtual bool bufferOne(void*) noexcept {
        return false;
    }

    virtual bool unbufferOne(void**) noexcept {
        return false;
    }

    __attribute__((noinline)) void enqueueSlow(void* v) noexcept;
    __attribute__((noinline)) bool dequeueSlow(void** out) noexcept;

    void enqueue(void* v) noexcept;
    bool dequeue(void** out) noexcept;

    bool tryEnqueue(void* v) noexcept;
    bool tryDequeue(void** out) noexcept;

    void close() noexcept;

    virtual ~Impl() noexcept {
    }
};

bool Channel::Impl::sendOne(void* v) noexcept {
    if (auto w = (Waiter*)receivers_.popFrontOrNull(); w) {
        w->value = v;
        w->valueSet = true;
        w->ev->signal();

        return true;
    }

    return bufferOne(v);
}

bool Channel::Impl::recvOne(void** out) noexcept {
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

void Channel::Impl::enqueue(void* v) noexcept {
    mu_.lock();

    STD_INSIST(!closed_);

    if (sendOne(v)) {
        mu_.unlock();
        return;
    }

    enqueueSlow(v);
}

bool Channel::Impl::dequeue(void** out) noexcept {
    mu_.lock();

    if (recvOne(out)) {
        mu_.unlock();
        return true;
    }

    if (closed_) {
        mu_.unlock();
        return false;
    }

    return dequeueSlow(out);
}

bool Channel::Impl::tryEnqueue(void* v) noexcept {
    LockGuard guard(mu_);

    STD_INSIST(!closed_);

    return sendOne(v);
}

bool Channel::Impl::tryDequeue(void** out) noexcept {
    LockGuard guard(mu_);

    return recvOne(out);
}

void Channel::Impl::close() noexcept {
    LockGuard guard(mu_);

    STD_INSIST(!closed_);
    STD_INSIST(senders_.empty());

    closed_ = true;

    while (auto w = (Waiter*)receivers_.popFrontOrNull()) {
        w->ev->signal();
    }
}

__attribute__((noinline)) void Channel::Impl::enqueueSlow(void* v) noexcept {
    Event ev(exec_);
    Waiter w;

    w.ev = &ev;
    w.value = v;
    w.valueSet = true;

    senders_.pushBack(&w);
    ev.wait(makeRunable([this] {
        mu_.unlock();
    }));
}

__attribute__((noinline)) bool Channel::Impl::dequeueSlow(void** out) noexcept {
    Event ev(exec_);
    Waiter w;

    w.ev = &ev;
    w.value = nullptr;
    w.valueSet = false;

    receivers_.pushBack(&w);
    ev.wait(makeRunable([this] {
        mu_.unlock();
    }));

    if (w.valueSet) {
        *out = w.value;

        return true;
    }

    return false;
}

namespace {
    struct Pow2RingBuf {
        void** buf_;
        size_t mask_;
        size_t head_;
        size_t size_;

        Pow2RingBuf(void** buf, size_t capa) noexcept;

        bool empty() const noexcept {
            return size_ == 0;
        }

        bool full() const noexcept {
            return size_ == mask_ + 1;
        }

        void push(void* v) noexcept;
        void* pop() noexcept;
    };

    struct BufferedImpl: public Channel::Impl {
        Pow2RingBuf buf_;

        BufferedImpl(size_t capa) noexcept
            : buf_((void**)(this + 1), capa)
        {
        }

        BufferedImpl(CoroExecutor* exec, size_t capa) noexcept
            : Impl(exec)
            , buf_((void**)(this + 1), capa)
        {
        }

        void* operator new(size_t, void* p) noexcept {
            return p;
        }

        void operator delete(void* p) noexcept {
            freeMemory(p);
        }

        bool bufferOne(void* v) noexcept override;
        bool unbufferOne(void** out) noexcept override;
    };
}

bool BufferedImpl::bufferOne(void* v) noexcept {
    if (!buf_.full()) {
        buf_.push(v);

        return true;
    }

    return false;
}

bool BufferedImpl::unbufferOne(void** out) noexcept {
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

Pow2RingBuf::Pow2RingBuf(void** buf, size_t capa) noexcept
    : buf_(buf)
    , mask_(capa - 1)
    , head_(0)
    , size_(0)
{
}

void Pow2RingBuf::push(void* v) noexcept {
    buf_[(head_ + size_) & mask_] = v;
    ++size_;
}

void* Pow2RingBuf::pop() noexcept {
    void* v = buf_[head_];
    head_ = (head_ + 1) & mask_;
    --size_;
    return v;
}

Channel::Channel()
    : impl_(new Impl())
{
}

Channel::Channel(size_t cap)
    : impl_(cap == 0
                ? new Impl()
                : new (allocateMemory(sizeof(BufferedImpl) + clp2(cap) * sizeof(void*))) BufferedImpl(clp2(cap)))
{
}

Channel::Channel(CoroExecutor* exec)
    : Channel(exec, 0)
{
}

Channel::Channel(CoroExecutor* exec, size_t cap)
    : impl_(cap == 0
                ? new Impl(exec)
                : new (allocateMemory(sizeof(BufferedImpl) + clp2(cap) * sizeof(void*))) BufferedImpl(exec, clp2(cap)))
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
