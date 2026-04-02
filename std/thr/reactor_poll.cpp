#include "reactor_poll.h"

#include "coro.h"
#include "pool.h"
#include "mutex.h"
#include "guard.h"
#include "thread.h"
#include "poller.h"

#include <std/mem/new.h>
#include <std/sys/crt.h>
#include <std/lib/list.h>
#include <std/lib/node.h>
#include <std/thr/task.h>
#include <std/map/treap.h>
#include <std/sym/i_map.h>
#include <std/alg/minmax.h>
#include <std/sys/atomic.h>
#include <std/alg/exchange.h>
#include <std/mem/obj_pool.h>
#include <std/sys/event_fd.h>
#include <std/map/treap_node.h>

using namespace stl;

namespace {
    struct DeadlineTreap: public Treap {
        bool cmp(void* a, void* b) const noexcept override;
        u64 earliest() const noexcept;
    };

    struct ReactorState;

    struct ReqCommon {
        u64 deadline = 0;
        Task* task = nullptr;
        ReactorState* reactor = nullptr;
    };

    struct InternalReq: public TreapNode, public IntrusiveNode {
        u32 fd = 0;
        u32 flags = 0;
        u32 result = 0;
        ReqCommon* common = nullptr;

        virtual void complete(u32 res, IntrusiveList& ready) noexcept;
        virtual ~InternalReq() = default;
    };

    struct InternalMultiReq: public InternalReq {
        InternalMultiReq* next = nullptr;

        void complete(u32 res, IntrusiveList& ready) noexcept override;
    };

    struct FdEntry: public IntrusiveList {
        u32 flags() const noexcept;
    };

    struct ReactorState: public ReactorIface, public Runable {
        CoroExecutor* exec_;
        ThreadPool* pool;
        PollerIface* poller;
        DeadlineTreap timers;
        DeadlineTreap sleepers;
        IntMap<FdEntry> fdMap_;
        Mutex queueMutex_;
        DeadlineTreap queue_;
        EventFD wakeEv_;
        bool stopped_ = false;
        Thread* thread_ = nullptr;

        ReactorState(CoroExecutor* exec, ThreadPool* p, ObjPool* opool);
        ~ReactorState() noexcept;

        void drainQueue();
        void wakeup() noexcept;
        void rearmOrDisarm(int fd);
        void drainWakeup() noexcept;
        void run() noexcept override;
        void cancelInternal(InternalReq* req);
        void processEvent(PollEvent* ev, IntrusiveList& ready) noexcept;

        u32 poll(int fd, u32 flags, u64 deadlineUs) override;
        size_t pollMulti(const PollFD* in, PollFD* out, size_t count, u64 deadlineUs) override;
    };
}

bool DeadlineTreap::cmp(void* a, void* b) const noexcept {
    auto ra = (InternalReq*)a;
    auto rb = (InternalReq*)b;

    if (ra->common->deadline != rb->common->deadline) {
        return ra->common->deadline < rb->common->deadline;
    }

    return ra < rb;
}

u64 DeadlineTreap::earliest() const noexcept {
    auto n = (InternalReq*)min();

    return n ? n->common->deadline : UINT64_MAX;
}

void InternalReq::complete(u32 res, IntrusiveList& ready) noexcept {
    result = res;
    ready.pushBack(common->task);
}

void InternalMultiReq::complete(u32 res, IntrusiveList& ready) noexcept {
    result = res;
    ready.pushBack(common->task);

    for (auto r = next; r != this; r = r->next) {
        common->reactor->cancelInternal(r);
    }
}

u32 FdEntry::flags() const noexcept {
    u32 f = 0;

    for (auto n = front(), e = end(); n != e; n = n->next) {
        f |= static_cast<const InternalReq*>(n)->flags;
    }

    return f;
}

ReactorState::ReactorState(CoroExecutor* exec, ThreadPool* p, ObjPool* opool)
    : exec_(exec)
    , pool(p)
    , poller(PollerIface::create(opool))
    , fdMap_(ObjPool::create(opool))
    , queueMutex_(Mutex::spinLock(exec))
{
    poller->arm(wakeEv_.fd(), PollFlag::In, nullptr);
    thread_ = opool->make<Thread>(*this);
}

void ReactorState::rearmOrDisarm(int fd) {
    if (auto entry = fdMap_.find(fd); !entry) {
        poller->disarm(fd);
    } else if (entry->empty()) {
        poller->disarm(fd);
        fdMap_.erase(fd);
    } else {
        poller->arm(fd, entry->flags(), (void*)(uintptr_t)(fd + 1));
    }
}

void ReactorState::cancelInternal(InternalReq* req) {
    req->remove();
    timers.remove(req);
    rearmOrDisarm(req->fd);
}

void ReactorState::wakeup() noexcept {
    wakeEv_.signal();
}

ReactorState::~ReactorState() noexcept {
    stdAtomicStore(&stopped_, true, MemoryOrder::Release);
    wakeup();
    thread_->join();
}

void ReactorState::drainWakeup() noexcept {
    wakeEv_.drain();
}

void ReactorState::drainQueue() {
    DeadlineTreap local;

    LockGuard(queueMutex_).run([this, &local] {
        queue_.xchg(local);
    });

    local.visit([&](TreapNode* node) {
        auto req = (InternalReq*)node;

        req->left = nullptr;
        req->right = nullptr;

        if (req->fd == (u32)-1) {
            sleepers.insert(req);
        } else {
            timers.insert(req);

            auto& entry = fdMap_[req->fd];

            entry.pushBack(req);
            poller->arm(req->fd, entry.flags(), (void*)(uintptr_t)(req->fd + 1));
        }
    });
}

void ReactorState::processEvent(PollEvent* ev, IntrusiveList& ready) noexcept {
    int fd = (uintptr_t)ev->data - 1;

    if (auto entry = fdMap_.find(fd); entry) {
        for (auto n = entry->mutFront(), e = entry->mutEnd(); n != e;) {
            if (auto req = (InternalReq*)exchange(n, n->next); req->flags & ev->flags) {
                req->remove();
                timers.remove(req);
                req->complete(ev->flags, ready);
            }
        }

        rearmOrDisarm(fd);
    }
}

void ReactorState::run() noexcept {
    while (!stdAtomicFetch(&stopped_, MemoryOrder::Acquire)) {
        drainQueue();

        IntrusiveList ready;

        poller->wait([this, &ready](PollEvent* ev) {
            if (ev->data) {
                processEvent(ev, ready);
            } else {
                drainWakeup();
                poller->arm(wakeEv_.fd(), PollFlag::In, nullptr);
            }
        }, min(timers.earliest(), sleepers.earliest()));

        auto now = monotonicNowUs();

        while (auto req = (InternalReq*)timers.min()) {
            if (req->common->deadline > now) {
                break;
            }

            timers.remove(req);
            req->remove();
            rearmOrDisarm(req->fd);
            req->complete(0, ready);
        }

        while (auto req = (InternalReq*)sleepers.min()) {
            if (req->common->deadline > now) {
                break;
            }

            sleepers.remove(req);
            req->complete(0, ready);
        }

        if (!ready.empty()) {
            pool->submitTasks(ready);
        }
    }
}

u32 ReactorState::poll(int fd, u32 flags, u64 deadlineUs) {
    ReqCommon common;

    common.deadline = deadlineUs;
    common.reactor = this;

    InternalReq req;

    req.fd = (u32)fd;
    req.flags = flags;
    req.common = &common;

    queueMutex_.lock();
    queue_.insert(&req);

    // clang-format off
    exec_->parkWith(makeRunable([this, needsWakeup = (queue_.min() == &req)] {
        queueMutex_.unlock();

        if (needsWakeup) {
            wakeup();
        }
    }), &common.task);
    // clang-format on

    return req.result;
}

size_t ReactorState::pollMulti(const PollFD* in, PollFD* out, size_t count, u64 deadlineUs) {
    if (count == 0) {
        poll(-1, 0, deadlineUs);

        return 0;
    }

    auto opool = ObjPool::fromMemory();
    auto p = opool.mutPtr();
    auto reqs = (InternalMultiReq**)p->allocate(sizeof(InternalMultiReq*) * count);

    ReqCommon common;

    common.deadline = deadlineUs;
    common.reactor = this;

    InternalMultiReq* last = nullptr;

    for (size_t i = 0; i < count; ++i) {
        auto req = p->make<InternalMultiReq>();

        req->fd = (u32)in[i].fd;
        req->flags = in[i].flags;
        req->common = &common;
        req->next = last;

        last = req;
        reqs[i] = req;
    }

    reqs[0]->next = last;

    queueMutex_.lock();

    for (size_t i = 0; i < count; ++i) {
        queue_.insert(reqs[i]);
    }

    // clang-format off
    exec_->parkWith(makeRunable([this] {
        queueMutex_.unlock();
        wakeup();
    }), &common.task);
    // clang-format on

    size_t nout = 0;

    for (size_t i = 0; i < count; ++i) {
        if (reqs[i]->result) {
            out[nout++] = {in[i].fd, reqs[i]->result};
        }
    }

    return nout;
}

ReactorIface* ReactorIface::create(CoroExecutor* exec, ThreadPool* pool, ObjPool* opool) {
    return opool->make<ReactorState>(exec, pool, opool);
}
