#include "reactor_poll.h"

#include "pool.h"
#include "mutex.h"
#include "guard.h"
#include "thread.h"
#include "poller.h"

#include <std/mem/new.h>
#include <std/sys/event_fd.h>
#include <std/sys/crt.h>
#include <std/lib/list.h>
#include <std/map/treap.h>
#include <std/sym/i_map.h>
#include <std/alg/minmax.h>
#include <std/sys/atomic.h>
#include <std/alg/exchange.h>
#include <std/mem/obj_pool.h>

using namespace stl;

namespace {
    struct DeadlineTreap: public Treap {
        bool cmp(void* a, void* b) const noexcept override {
            auto ra = (PollRequest*)a;
            auto rb = (PollRequest*)b;

            if (ra->deadline != rb->deadline) {
                return ra->deadline < rb->deadline;
            }

            return ra < rb;
        }

        u64 earliest() const noexcept {
            auto n = (PollRequest*)min();

            return n ? n->deadline : UINT64_MAX;
        }
    };

    struct FdEntry: public IntrusiveList {
        u32 flags() const noexcept {
            u32 f = 0;

            for (auto n = front(), e = end(); n != e; n = n->next) {
                f |= static_cast<const PollRequest*>(n)->flags;
            }

            return f;
        }
    };

    struct ReactorState: public ReactorIface, public Runable {
        ThreadPool* pool;
        PollerIface* poller;
        DeadlineTreap timers;
        DeadlineTreap sleepers;
        ObjPool::Ref fdPool_ = ObjPool::fromMemory();
        IntMap<FdEntry> fdMap_{fdPool_.mutPtr()};
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
        void processRequest(PollRequest* req) override;
        void processEvent(PollEvent* ev, IntrusiveList& ready) noexcept;
    };
}

ReactorState::ReactorState(CoroExecutor* exec, ThreadPool* p, ObjPool* opool)
    : pool(p)
    , poller(PollerIface::create(opool))
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

void ReactorState::processRequest(PollRequest* req) {
    queueMutex_.lock();
    queue_.insert(req);

    req->parkWith(makeRunable([this, needsWakeup = (queue_.min() == req)] {
        queueMutex_.unlock();

        if (needsWakeup) {
            wakeup();
        }
    }));
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
        auto req = (PollRequest*)node;

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
            if (auto req = (PollRequest*)exchange(n, n->next); req->flags & ev->flags) {
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

        while (auto req = (PollRequest*)timers.min()) {
            if (req->deadline > now) {
                break;
            }

            timers.remove(req);
            req->remove();
            rearmOrDisarm(req->fd);
            req->complete(0, ready);
        }

        while (auto req = (PollRequest*)sleepers.min()) {
            if (req->deadline > now) {
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

ReactorIface* ReactorIface::create(CoroExecutor* exec, ThreadPool* pool, ObjPool* opool) {
    return opool->make<ReactorState>(exec, pool, opool);
}
