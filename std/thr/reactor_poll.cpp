#include "reactor_poll.h"

#include "coro.h"
#include "pool.h"
#include "mutex.h"
#include "guard.h"
#include "thread.h"
#include "parker.h"
#include "poller.h"
#include "poll_fd.h"

#include <std/mem/new.h>
#include <std/sys/crt.h>
#include <std/sys/crt.h>
#include <std/lib/list.h>
#include <std/lib/node.h>
#include <std/thr/task.h>
#include <std/map/treap.h>
#include <std/sym/i_map.h>
#include <std/alg/minmax.h>
#include <std/sys/atomic.h>
#include <std/dbg/assert.h>
#include <std/alg/exchange.h>
#include <std/mem/obj_pool.h>
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
        PollFD pfd = {};
        u32* result = nullptr;
        ReqCommon* common = nullptr;

        virtual void complete(u32 res, IntrusiveList& ready) noexcept;
    };

    struct InternalMultiReq: public InternalReq {
        InternalMultiReq* next = nullptr;

        void complete(u32 res, IntrusiveList& ready) noexcept override;
    };

    struct PollGroupImpl: public PollGroup {
        ReqCommon common_;
        InternalMultiReq** reqs_;
        u32* results_;
        PollFD* pfds_;
        size_t count_;

        PollGroupImpl(ObjPool* pool, const PollFD* fds, size_t count);

        int fd() const noexcept override;
        void reset(ReactorState* reactor, u64 deadlineUs) noexcept;
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
        Parker parker_;
        bool stopped_ = false;
        Thread* thread_ = nullptr;

        ReactorState(CoroExecutor* exec, ThreadPool* p, ObjPool* opool);
        ~ReactorState() noexcept;

        void drainQueue();
        void rearmOrDisarm(int fd);
        void run() noexcept override;
        void cancelInternal(InternalReq* req);
        void processEvent(PollFD* ev, IntrusiveList& ready) noexcept;

        size_t poll(PollGroup* g, PollFD* out, u64 deadlineUs) override;
        u32 poll(PollFD pfd, u64 deadlineUs) override;
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
    *result = res;
    ready.pushBack(common->task);
}

void InternalMultiReq::complete(u32 res, IntrusiveList& ready) noexcept {
    *result = res;
    ready.pushBack(common->task);

    for (auto r = next; r != this; r = r->next) {
        common->reactor->cancelInternal(r);
    }
}

u32 FdEntry::flags() const noexcept {
    u32 f = 0;

    for (auto n = front(), e = end(); n != e; n = n->next) {
        f |= static_cast<const InternalReq*>(n)->pfd.flags;
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
    poller->arm({parker_.fd(), PollFlag::In});
    thread_ = opool->make<Thread>(*this);
}

void ReactorState::rearmOrDisarm(int fd) {
    if (auto entry = fdMap_.find(fd); !entry) {
        poller->disarm(fd);
    } else if (entry->empty()) {
        poller->disarm(fd);
        fdMap_.erase(fd);
    } else {
        poller->arm({fd, entry->flags()});
    }
}

void ReactorState::cancelInternal(InternalReq* req) {
    req->remove();
    timers.remove(req);
    rearmOrDisarm(req->pfd.fd);
}

ReactorState::~ReactorState() noexcept {
    stdAtomicStore(&stopped_, true, MemoryOrder::Release);
    parker_.signal();
    thread_->join();
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

        int fd = req->pfd.fd;

        if (fd == -1) {
            sleepers.insert(req);
        } else {
            timers.insert(req);

            auto& entry = fdMap_[fd];

            entry.pushBack(req);
            poller->arm({fd, entry.flags()});
        }
    });
}

void ReactorState::processEvent(PollFD* ev, IntrusiveList& ready) noexcept {
    if (auto entry = fdMap_.find(ev->fd); entry) {
        for (auto n = entry->mutFront(), e = entry->mutEnd(); n != e;) {
            if (auto req = (InternalReq*)exchange(n, n->next); req->pfd.flags & ev->flags) {
                req->remove();
                timers.remove(req);
                req->complete(ev->flags, ready);
            }
        }

        rearmOrDisarm(ev->fd);
    }
}

void ReactorState::run() noexcept {
    while (!stdAtomicFetch(&stopped_, MemoryOrder::Acquire)) {
        IntrusiveList ready;

        parker_.park([&] {
            drainQueue();

            poller->wait([this, &ready](PollFD* ev) {
                if (ev->fd == parker_.fd()) {
                    parker_.drain();
                    poller->arm({parker_.fd(), PollFlag::In});
                } else {
                    processEvent(ev, ready);
                }
            }, min(timers.earliest(), sleepers.earliest()));
        });

        auto now = monotonicNowUs();

        while (auto req = (InternalReq*)timers.min()) {
            if (req->common->deadline > now) {
                break;
            }

            timers.remove(req);
            req->remove();
            rearmOrDisarm(req->pfd.fd);
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

PollGroupImpl::PollGroupImpl(ObjPool* pool, const PollFD* fds, size_t count)
    : reqs_((InternalMultiReq**)pool->allocate(sizeof(InternalMultiReq*) * count))
    , results_((u32*)pool->allocate(sizeof(u32) * count))
    , pfds_((PollFD*)pool->allocate(sizeof(PollFD) * count))
    , count_(count)
{
    for (size_t i = 0; i < count; ++i) {
        auto req = pool->make<InternalMultiReq>();

        req->pfd = fds[i];
        req->result = &results_[i];
        req->common = &common_;
        req->next = (i > 0) ? reqs_[i - 1] : nullptr;

        reqs_[i] = req;
        pfds_[i] = fds[i];
        results_[i] = 0;
    }

    reqs_[0]->next = reqs_[count - 1];
}

int PollGroupImpl::fd() const noexcept {
    return pfds_[0].fd;
}

void PollGroupImpl::reset(ReactorState* reactor, u64 deadlineUs) noexcept {
    common_.reactor = reactor;
    common_.deadline = deadlineUs;
    common_.task = nullptr;

    memZero(results_, results_ + count_);
}

PollGroup* PollGroup::create(ObjPool* pool, const PollFD* fds, size_t count) {
    return pool->make<PollGroupImpl>(pool, fds, count);
}

size_t ReactorState::poll(PollGroup* g, PollFD* out, u64 deadlineUs) {
    auto impl = (PollGroupImpl*)g;

    impl->reset(this, deadlineUs);

    queueMutex_.lock();

    for (size_t i = 0; i < impl->count_; ++i) {
        queue_.insert(impl->reqs_[i]);
    }

    // clang-format off
    exec_->parkWith(makeRunable([this] {
        queueMutex_.unlock();
        parker_.unpark();
    }), &impl->common_.task);
    // clang-format on

    size_t nout = 0;

    for (size_t i = 0; i < impl->count_; ++i) {
        if (impl->results_[i]) {
            out[nout++] = {impl->pfds_[i].fd, impl->results_[i]};
        }
    }

    return nout;
}

u32 ReactorState::poll(PollFD pfd, u64 deadlineUs) {
    ReqCommon common;

    common.deadline = deadlineUs;
    common.reactor = this;

    u32 resultVal = 0;
    InternalReq req;

    req.pfd = pfd;
    req.result = &resultVal;
    req.common = &common;

    queueMutex_.lock();
    queue_.insert(&req);

    // clang-format off
    exec_->parkWith(makeRunable([this, needsWakeup = (queue_.min() == &req)] {
        queueMutex_.unlock();

        if (needsWakeup) {
            parker_.unpark();
        }
    }), &common.task);
    // clang-format on

    return resultVal;
}

ReactorIface* ReactorIface::create(CoroExecutor* exec, ThreadPool* pool, ObjPool* opool) {
    return opool->make<ReactorState>(exec, pool, opool);
}
