#include "channel.h"
#include "event.h"
#include "mutex.h"
#include "guard.h"

#include <std/sys/crt.h>
#include <std/alg/defer.h>
#include <std/mem/obj_pool.h>
#include <std/lib/list.h>
#include <std/dbg/insist.h>
#include <std/alg/bits.h>

using namespace stl;

namespace {
    struct Waiter: public IntrusiveNode {
        Event* ev;
        void* value;
        bool valueSet;
    };

    struct ChannelImpl: public Channel {
        CoroExecutor* exec_;
        Mutex* mu_;
        IntrusiveList senders_;
        IntrusiveList receivers_;
        bool closed_;

        ChannelImpl(Mutex* mu) noexcept
            : exec_(nullptr)
            , mu_(mu)
            , closed_(false)
        {
        }

        ChannelImpl(CoroExecutor* exec, Mutex* mu) noexcept
            : exec_(exec)
            , mu_(mu)
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

        void enqueue(void* v) noexcept override;
        void enqueue(void** from, size_t len) noexcept override;
        bool dequeue(void** out) noexcept override;
        size_t dequeue(void** to, size_t len) noexcept override;

        bool tryEnqueue(void* v) noexcept override;
        bool tryDequeue(void** out) noexcept override;

        void close() noexcept override;
    };

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

    struct BufferedImpl: public ChannelImpl {
        Pow2RingBuf buf_;

        BufferedImpl(Mutex* mu, void** storage, size_t capa) noexcept
            : ChannelImpl(mu)
            , buf_(storage, capa)
        {
        }

        BufferedImpl(CoroExecutor* exec, Mutex* mu, void** storage, size_t capa) noexcept
            : ChannelImpl(exec, mu)
            , buf_(storage, capa)
        {
        }

        bool bufferOne(void* v) noexcept override;
        bool unbufferOne(void** out) noexcept override;
    };
}

bool ChannelImpl::sendOne(void* v) noexcept {
    if (auto w = (Waiter*)receivers_.popFrontOrNull(); w) {
        w->value = v;
        w->valueSet = true;
        w->ev->signal();

        return true;
    }

    return bufferOne(v);
}

void ChannelImpl::enqueue(void* v) noexcept {
    mu_->lock();

    STD_INSIST(!closed_);

    if (sendOne(v)) {
        mu_->unlock();
        return;
    }

    enqueueSlow(v);
}

void ChannelImpl::enqueue(void** from, size_t len) noexcept {
    for (size_t i = 0; i < len;) {
        mu_->lock();

        STD_INSIST(!closed_);

        if (sendOne(from[i])) {
            ++i;

            while (i < len && sendOne(from[i])) {
                ++i;
            }

            mu_->unlock();
        } else {
            enqueueSlow(from[i]);
            ++i;
        }
    }
}

bool ChannelImpl::tryEnqueue(void* v) noexcept {
    LockGuard guard(*mu_);

    STD_INSIST(!closed_);

    return sendOne(v);
}

__attribute__((noinline)) void ChannelImpl::enqueueSlow(void* v) noexcept {
    Event::Buf buf;
    auto* ev = Event::create(buf, exec_);
    STD_DEFER {
        delete ev;
    };

    Waiter w;

    w.ev = ev;
    w.value = v;
    w.valueSet = true;

    senders_.pushBack(&w);
    ev->wait(makeRunable([this] {
        mu_->unlock();
    }));
}

bool ChannelImpl::recvOne(void** out) noexcept {
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

bool ChannelImpl::dequeue(void** out) noexcept {
    mu_->lock();

    if (recvOne(out)) {
        mu_->unlock();
        return true;
    }

    if (closed_) {
        mu_->unlock();
        return false;
    }

    return dequeueSlow(out);
}

bool ChannelImpl::tryDequeue(void** out) noexcept {
    LockGuard guard(*mu_);

    return recvOne(out);
}

__attribute__((noinline)) bool ChannelImpl::dequeueSlow(void** out) noexcept {
    Event::Buf buf;
    auto* ev = Event::create(buf, exec_);
    STD_DEFER {
        delete ev;
    };

    Waiter w;

    w.ev = ev;
    w.value = nullptr;
    w.valueSet = false;

    receivers_.pushBack(&w);
    ev->wait(makeRunable([this] {
        mu_->unlock();
    }));

    if (w.valueSet) {
        *out = w.value;

        return true;
    }

    return false;
}

size_t ChannelImpl::dequeue(void** to, size_t len) noexcept {
    if (len == 0) {
        return 0;
    }

    // block for the first element
    if (!dequeue(to)) {
        return 0;
    }

    size_t n = 1;

    // drain the rest under one lock
    mu_->lock();

    while (n < len && recvOne(to + n)) {
        ++n;
    }

    mu_->unlock();

    return n;
}

void ChannelImpl::close() noexcept {
    LockGuard guard(*mu_);

    STD_INSIST(!closed_);
    STD_INSIST(senders_.empty());

    closed_ = true;

    while (auto w = (Waiter*)receivers_.popFrontOrNull()) {
        w->ev->signal();
    }
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

Channel* Channel::create(ObjPool* pool) {
    return pool->make<ChannelImpl>(Mutex::create(pool));
}

Channel* Channel::create(ObjPool* pool, size_t cap) {
    if (cap == 0) {
        return create(pool);
    }

    size_t rcap = clp2(cap);
    auto storage = (void**)pool->allocate(rcap * sizeof(void*));

    return pool->make<BufferedImpl>(Mutex::create(pool), storage, rcap);
}

Channel* Channel::create(ObjPool* pool, CoroExecutor* exec) {
    return pool->make<ChannelImpl>(exec, Mutex::createSpinLock(pool, exec));
}

Channel* Channel::create(ObjPool* pool, CoroExecutor* exec, size_t cap) {
    if (cap == 0) {
        return create(pool, exec);
    }

    size_t rcap = clp2(cap);
    auto storage = (void**)pool->allocate(rcap * sizeof(void*));

    return pool->make<BufferedImpl>(exec, Mutex::createSpinLock(pool, exec), storage, rcap);
}
