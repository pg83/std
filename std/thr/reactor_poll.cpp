#include "reactor_poll.h"

#include "coro.h"
#include "pool.h"
#include "mutex.h"
#include "guard.h"
#include "thread.h"
#include "parker.h"
#include "poller.h"
#include "poll_fd.h"
#include "io_reactor.h"

#include <std/mem/new.h>
#include <std/sys/crt.h>
#include <std/lib/list.h>
#include <std/lib/node.h>
#include <std/thr/task.h>
#include <std/map/treap.h>
#include <std/sym/i_map.h>
#include <std/alg/range.h>
#include <std/alg/minmax.h>
#include <std/sys/atomic.h>
#include <std/dbg/assert.h>
#include <std/lib/visitor.h>
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
    struct ReactorPoller;

    struct ReqCommon {
        u64 deadline = 0;
        Task* task = nullptr;
        ReactorState* reactor = nullptr;

        void reset(ReactorState* r, u64 dl) noexcept;
    };

    struct InternalReq: public TreapNode, public IntrusiveNode {
        PollFD pfd = {};
        u32 result = 0;
        ReqCommon* common = nullptr;

        virtual void complete(u32 res, IntrusiveList& ready) noexcept;
    };

    struct InternalMultiReq: public InternalReq {
        void complete(u32 res, IntrusiveList& ready) noexcept override;
    };

    struct FdEntry: public IntrusiveList {
        u32 flags() const noexcept;
    };

    struct ReactorState: public ReactorIface, public Runable {
        CoroExecutor* exec_;
        PollerIface* poller;
        DeadlineTreap timers;
        DeadlineTreap sleepers;
        IntMap<FdEntry> fdMap_;
        Mutex* queueMutex_;
        DeadlineTreap queue_;
        Parker parker_;
        bool stopped_ = false;
        Thread* thread_ = nullptr;

        ReactorState(CoroExecutor* exec, ObjPool* opool);
        ~ReactorState() noexcept;

        void drainQueue();
        void rearmOrDisarm(int fd);
        void run() noexcept override;
        void cancelInternal(InternalReq* req);
        void processEvent(PollFD* ev, IntrusiveList& ready) noexcept;

        u32 poll(PollFD pfd, u64 deadlineUs) override;
        PollerIface* createPoller(ObjPool* pool) override;
    };

    struct ReactorPoller: public PollerIface, public ReqCommon {
        IntMap<InternalMultiReq> fds_;

        ReactorPoller(ObjPool* pool, ReactorState* rs);

        void arm(PollFD pfd) override;
        void disarm(int fd) override;
        void waitImpl(VisitorFace& v, u32 timeoutUs) override;
        void cancelOthers(InternalMultiReq* except);
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

void ReqCommon::reset(ReactorState* r, u64 dl) noexcept {
    reactor = r;
    deadline = dl;
    task = nullptr;
}

void InternalReq::complete(u32 res, IntrusiveList& ready) noexcept {
    result = res;
    ready.pushBack(common->task);
}

void InternalMultiReq::complete(u32 res, IntrusiveList& ready) noexcept {
    result = res;
    ready.pushBack(common->task);
    ((ReactorPoller*)common)->cancelOthers(this);
}

u32 FdEntry::flags() const noexcept {
    u32 f = 0;

    for (auto n = front(), e = end(); n != e; n = n->next) {
        f |= static_cast<const InternalReq*>(n)->pfd.flags;
    }

    return f;
}

ReactorState::ReactorState(CoroExecutor* exec, ObjPool* opool)
    : exec_(exec)
    , poller(PollerIface::create(opool))
    , fdMap_(ObjPool::create(opool))
    , queueMutex_(Mutex::createSpinLock(opool, exec))
{
    poller->arm({parker_.fd(), PollFlag::In});
    thread_ = Thread::create(opool, *this);
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

    LockGuard(*queueMutex_).run([this, &local] {
        queue_.xchg(local);
    });

    local.visit([&](TreapNode* node) {
        auto req = (InternalReq*)node;

        req->left = nullptr;
        req->right = nullptr;

        if (int fd = req->pfd.fd; fd == -1) {
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
            exec_->pool()->submitTasks(ready);
        }
    }
}

u32 ReactorState::poll(PollFD pfd, u64 deadlineUs) {
    ReqCommon common;

    common.reset(this, deadlineUs);

    InternalReq req;

    req.pfd = pfd;
    req.common = &common;

    queueMutex_->lock();
    queue_.insert(&req);

    // clang-format off
    exec_->parkWith(makeRunable([this, needsWakeup = (queue_.min() == &req)] {
        queueMutex_->unlock();

        if (needsWakeup) {
            parker_.unpark();
        }
    }), &common.task);
    // clang-format on

    return req.result;
}

PollerIface* ReactorState::createPoller(ObjPool* pool) {
    if (auto p = WaitablePoller::create(pool, this); p) {
        return p;
    }

    return pool->make<ReactorPoller>(pool, this);
}

// ReactorPoller

ReactorPoller::ReactorPoller(ObjPool* pool, ReactorState* rs)
    : fds_(pool)
{
    reactor = rs;
}

void ReactorPoller::arm(PollFD pfd) {
    fds_[pfd.fd].pfd = pfd;
}

void ReactorPoller::disarm(int fd) {
    fds_.erase(fd);
}

void ReactorPoller::waitImpl(VisitorFace& v, u32 timeoutUs) {
    auto rs = reactor;
    auto dl = monotonicNowUs() + (u64)timeoutUs;

    reset(rs, dl);

    fds_.visit([](InternalMultiReq& req) {
        req.result = 0;
        req.left = nullptr;
        req.right = nullptr;
    });

    rs->queueMutex_->lock();

    fds_.visit([this, rs](InternalMultiReq& req) {
        req.common = this;
        rs->queue_.insert(&req);
    });

    // clang-format off
    rs->exec_->parkWith(makeRunable([rs] {
        rs->queueMutex_->unlock();
        rs->parker_.unpark();
    }), &task);
    // clang-format on

    fds_.visit([&v](InternalMultiReq& req) {
        if (auto res = req.result; res) {
            PollFD pfd = {req.pfd.fd, res};

            v.visit(&pfd);
        }
    });
}

void ReactorPoller::cancelOthers(InternalMultiReq* except) {
    fds_.visit([this, except](InternalMultiReq& req) {
        if (&req != except) {
            reactor->cancelInternal(&req);
        }
    });
}

ReactorIface* ReactorIface::create(CoroExecutor* exec, ObjPool* opool) {
    return opool->make<ReactorState>(exec, opool);
}
