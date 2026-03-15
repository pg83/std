#include "reactor.h"

#include "pool.h"
#include "coro.h"
#include "mutex.h"
#include "poller.h"
#include "event.h"

#include <std/sys/fd.h>
#include <std/mem/new.h>
#include <std/sys/crt.h>
#include <std/lib/list.h>
#include <std/map/treap.h>
#include <std/sym/i_map.h>
#include <std/alg/minmax.h>
#include <std/mem/obj_pool.h>

#include <unistd.h>

using namespace stl;

namespace {
    struct DeadlineTreap: public Treap {
        bool cmp(void* a, void* b) const noexcept override {
            auto* ra = (PollRequest*)a;
            auto* rb = (PollRequest*)b;

            if (ra->deadline != rb->deadline) {
                return ra->deadline < rb->deadline;
            }

            return ra < rb;
        }

        u64 earliest() const noexcept {
            auto* n = min();

            return n ? ((PollRequest*)n)->deadline : UINT64_MAX;
        }
    };

    struct FdEntry: public IntrusiveList {
        u32 flags() const noexcept {
            u32 f = 0;

            for (auto* n = front(); n != end(); n = n->next) {
                f |= static_cast<const PollRequest*>(n)->flags;
            }

            return f;
        }
    };

    struct ReactorState: public ReactorIface, public Newable {
        CoroExecutor* exec;
        ThreadPool* pool;
        PollerIface* poller;
        DeadlineTreap timers;
        IntMap<FdEntry> fdMap_;
        Mutex timerMutex;
        Event done_;
        ScopedFD wakeReadFd;
        ScopedFD wakeWriteFd;

        ReactorState(CoroExecutor* e, ThreadPool* p, ObjPool* opool);

        ~ReactorState() noexcept override = default;

        void wakeup() noexcept;
        void rearmOrDisarm(int fd);
        void drainWakeup() noexcept;
        void run() noexcept override;
        void join() noexcept override;
        void processRequest(PollRequest* req) override;
    };
}

ReactorState::ReactorState(CoroExecutor* e, ThreadPool* p, ObjPool* opool)
    : exec(e)
    , pool(p)
    , poller(PollerIface::create(opool))
    , timerMutex(e)
{
    createPipeFD(wakeReadFd, wakeWriteFd);
    wakeReadFd.setNonBlocking();
    poller->arm(wakeReadFd.get(), PollFlag::In, nullptr);
}

void ReactorState::rearmOrDisarm(int fd) {
    if (auto* entry = fdMap_.find(fd); !entry) {
        poller->disarm(fd);
    } else if (entry->empty()) {
        poller->disarm(fd);
        fdMap_.erase(fd);
    } else {
        poller->arm(fd, entry->flags(), (void*)(uintptr_t)(fd + 1));
    }
}

void ReactorState::processRequest(PollRequest* req) {
    timerMutex.lock();

    const auto prevEarliest = timers.earliest();

    timers.insert(req);

    auto& entry = fdMap_[req->fd];

    entry.pushBack(req);

    const auto fl = entry.flags();
    const auto fd = req->fd;
    const auto needsWakeup = req->deadline < prevEarliest;

    req->parkWith(makeRunablePtr([this, fd, fl, needsWakeup]() {
        poller->arm(fd, fl, (void*)(uintptr_t)(fd + 1));

        timerMutex.unlock();

        if (needsWakeup) {
            wakeup();
        }
    }));
}

void ReactorState::wakeup() noexcept {
    char b = 1;

    ::write(wakeWriteFd.get(), &b, 1);
}

void ReactorState::join() noexcept {
    exec = nullptr;
    wakeup();
    done_.wait();
}

void ReactorState::drainWakeup() noexcept {
    char buf[64];

    while (::read(wakeReadFd.get(), buf, sizeof(buf)) > 0) {
    }
}

void ReactorState::run() noexcept {
    while (auto* e = exec) {
        const u64 earliest = LockGuard(timerMutex).run([this]() {
            return timers.earliest();
        });

        pool->beforeBlock();

        poller->wait([this](PollEvent* ev) {
            if (ev->data == nullptr) {
                drainWakeup();
                poller->arm(wakeReadFd.get(), PollFlag::In, nullptr);
            } else {
                int fd = (uintptr_t)ev->data - 1;

                LockGuard(timerMutex).run([this, fd, evFlags = ev->flags]() {
                    if (auto* entry = fdMap_.find(fd); entry) {
                        for (auto n = entry->mutFront(), e = entry->mutEnd(); n != e;) {
                            auto* next = n->next;

                            if (auto* req = (PollRequest*)n; req->flags & evFlags) {
                                n->remove();
                                timers.remove(req);
                                req->complete(evFlags);
                            }

                            n = next;
                        }

                        rearmOrDisarm(fd);
                    }
                });
            }
        }, earliest);

        LockGuard(timerMutex).run([this, now = monotonicNowUs()]() {
            while (auto* node = timers.min()) {
                auto* req = (PollRequest*)node;

                if (req->deadline > now) {
                    break;
                }

                timers.remove(req);
                req->remove();

                rearmOrDisarm(req->fd);

                req->complete(0);
            }
        });

        e->yield();
    }

    done_.set();
}

ReactorIface::~ReactorIface() noexcept {
}

ReactorIface* ReactorIface::create(CoroExecutor* exec, ThreadPool* pool, ObjPool* opool) {
    return opool->make<ReactorState>(exec, pool, opool);
}
