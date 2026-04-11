#include "io_uring.h"
#include "coro.h"
#include "pool.h"
#include "mutex.h"
#include "poll_fd.h"
#include "io_reactor.h"
#include "cond_var_iface.h"

#include <std/sys/crt.h>
#include <std/lib/list.h>
#include <std/lib/vector.h>
#include <std/lib/visitor.h>
#include <std/mem/obj_pool.h>

#if __has_include(<liburing.h>)
    #include <liburing.h>
#endif

#include <errno.h>
#include <stdlib.h>
#include <poll.h>
#include <sys/socket.h>

using namespace stl;

#if __has_include(<liburing.h>)
namespace {
    constexpr u64 WAKEUP_COOKIE = 0xCAFE;

    struct Ring;

    thread_local Ring* currentRing_ = nullptr;

    struct UringReqBase {
        virtual void complete(int result, IntrusiveList& ready) noexcept = 0;
    };

    static int toErrno(int res) noexcept {
        if (res >= 0) {
            return 0;
        }

        if (res == -ECANCELED) {
            return EAGAIN;
        }

        return -res;
    }

    struct UringReq: UringReqBase {
        Task* task;
        int res;

        void complete(int result, IntrusiveList& ready) noexcept override;
    };

    struct RecvReq: UringReq {};
    struct SendReq: UringReq {};
    struct WritevReq: UringReq {};
    struct AcceptReq: UringReq {};
    struct ConnectReq: UringReq {};
    struct PreadReq: UringReq {};
    struct PwriteReq: UringReq {};
    struct FsyncReq: UringReq {};
    struct PollReq: UringReq {};

    struct TimeoutReq: UringReq {
        __kernel_timespec ts;
    };

    struct UringPollGroup: public PollGroup {
        PollFD* fds_;
        size_t count_;

        UringPollGroup(ObjPool* pool, const PollFD* fds, size_t count);
    };

    static u32 toPollMask(u32 flags) noexcept {
        u32 r = 0;

        if (flags & PollFlag::In) {
            r |= POLLIN;
        }

        if (flags & PollFlag::Out) {
            r |= POLLOUT;
        }

        return r;
    }

    static u32 fromPollMask(u32 events) noexcept {
        u32 r = 0;

        if (events & POLLIN) {
            r |= PollFlag::In;
        }

        if (events & POLLOUT) {
            r |= PollFlag::Out;
        }

        if (events & POLLERR) {
            r |= PollFlag::Err;
        }

        if (events & POLLHUP) {
            r |= PollFlag::Hup;
        }

        return r;
    }

    static __kernel_timespec usToTimespec(u64 us) noexcept {
        __kernel_timespec ts;

        ts.tv_sec = us / 1000000;
        ts.tv_nsec = (us % 1000000) * 1000;

        return ts;
    }

    struct Ring: public io_uring {
        Ring();
        Ring(u32 flags);

        virtual ~Ring() noexcept;

        void enable() noexcept;
        void sendMsg(int targetFd) noexcept;

        virtual void wakeUp(int targetFd) noexcept;
    };

    struct ExternalRing: public Ring {
        Mutex mutex_;

        ExternalRing();

        void wakeUp(int targetFd) noexcept override;
    };

    struct UringReactorImpl;

    struct UringCondVarImpl: public CondVarIface {
        Ring* ring_;
        UringReactorImpl* reactor_;

        UringCondVarImpl(Ring* ring, UringReactorImpl* reactor) noexcept;

        void signal() noexcept override;
        void broadcast() noexcept override;
        void wait(Mutex& mutex) noexcept override;
    };

    struct UringReactorImpl: public IoReactor, public ThreadPoolHooks {
        Vector<Ring*> rings_;
        Ring* ext_;
        CoroExecutor* exec_;

        Ring* currentRing() noexcept;

        template <typename Req, typename F>
        void submit(Req& req, F prep, u64 deadlineUs) noexcept;

        template <typename Req, typename F>
        void submitReq(Req& req, F prep) noexcept;

        template <typename Req, typename F>
        void submitReq(Req& req, F prep, u64 deadlineUs) noexcept;

        UringReactorImpl(ObjPool* pool, CoroExecutor* exec, size_t threads);

        ThreadPoolHooks* hooks() override;
        CondVarIface* createCondVar(size_t index) override;

        int recv(int, size_t*, void*, size_t, u64) override;
        int send(int, size_t*, const void*, size_t, u64) override;
        int writev(int, size_t*, iovec*, size_t, u64) override;
        int accept(int, int*, sockaddr*, u32*, u64) override;
        int connect(int, const sockaddr*, u32, u64) override;
        int pread(int, size_t*, void*, size_t, off_t) override;
        int pwrite(int, size_t*, const void*, size_t, off_t) override;
        int fsync(int) override;
        int fdatasync(int) override;
        u32 poll(PollFD, u64) override;
        void poll(PollGroup*, VisitorFace&, u64) override;
        PollGroup* createPollGroup(ObjPool*, const PollFD*, size_t) override;
        void sleep(u64) override;
    };
}

void UringReq::complete(int result, IntrusiveList& ready) noexcept {
    res = result;
    ready.pushBack(task);
}

Ring::Ring()
    : Ring(IORING_SETUP_SINGLE_ISSUER | IORING_SETUP_DEFER_TASKRUN | IORING_SETUP_R_DISABLED)
{
}

Ring::Ring(u32 flags) {
    struct io_uring_params params = {};

    params.flags = flags;

    if (io_uring_queue_init_params(64, this, &params) < 0) {
        throw 1;
    }
}

Ring::~Ring() noexcept {
    io_uring_queue_exit(this);
}

void Ring::enable() noexcept {
    io_uring_enable_rings(this);
}

void Ring::sendMsg(int targetFd) noexcept {
    auto sqe = io_uring_get_sqe(this);

    io_uring_prep_msg_ring(sqe, targetFd, 0, WAKEUP_COOKIE, 0);
    sqe->flags |= IOSQE_CQE_SKIP_SUCCESS;
    io_uring_submit(this);
}

void Ring::wakeUp(int targetFd) noexcept {
    sendMsg(targetFd);
}

ExternalRing::ExternalRing()
    : Ring(0)
{
}

void ExternalRing::wakeUp(int targetFd) noexcept {
    mutex_.lock();
    sendMsg(targetFd);
    mutex_.unlock();
}

UringCondVarImpl::UringCondVarImpl(Ring* ring, UringReactorImpl* reactor) noexcept
    : ring_(ring)
    , reactor_(reactor)
{
}

void UringCondVarImpl::wait(Mutex& mutex) noexcept {
    if (!currentRing_) {
        ring_->enable();
        currentRing_ = ring_;
    }

    mutex.unlock();

    for (;;) {
        io_uring_submit_and_wait(ring_, 1);

        bool signaled = false;
        struct io_uring_cqe* cqe;
        IntrusiveList ready;

        while (io_uring_peek_cqe(ring_, &cqe) == 0) {
            auto ud = cqe->user_data;
            auto res = cqe->res;

            io_uring_cqe_seen(ring_, cqe);

            if (ud == WAKEUP_COOKIE) {
                signaled = true;
            } else {
                ((UringReqBase*)ud)->complete(res, ready);
            }
        }

        if (!ready.empty()) {
            reactor_->exec_->reSchedule(ready);
            reactor_->exec_->flushLocal();
        }

        if (signaled) {
            break;
        }
    }

    mutex.lock();
}

void UringCondVarImpl::signal() noexcept {
    auto src = reactor_->currentRing();
    src->wakeUp(ring_->ring_fd);
}

void UringCondVarImpl::broadcast() noexcept {
    signal();
}

UringReactorImpl::UringReactorImpl(ObjPool* pool, CoroExecutor* exec, size_t threads)
    : ext_(pool->make<ExternalRing>())
    , exec_(exec)
{
    for (size_t i = 0; i < threads; ++i) {
        rings_.pushBack(pool->make<Ring>());
    }
}

ThreadPoolHooks* UringReactorImpl::hooks() {
    return this;
}

Ring* UringReactorImpl::currentRing() noexcept {
    if (auto r = currentRing_; r) {
        return r;
    }

    size_t id;

    if (exec_->workerId(&id)) {
        auto r = rings_[id];

        r->enable();
        currentRing_ = r;

        return r;
    }

    return ext_;
}

template <typename Req, typename F>
void UringReactorImpl::submit(Req& req, F prep, u64 deadlineUs) noexcept {
    if (deadlineUs == UINT64_MAX) {
        submitReq(req, prep);
    } else {
        submitReq(req, prep, deadlineUs);
    }
}

template <typename Req, typename F>
void UringReactorImpl::submitReq(Req& req, F prep) noexcept {
    auto ring = currentRing();

    exec_->parkWith(makeRunable([&] {
        auto sqe = io_uring_get_sqe(ring);

        prep(sqe);

        io_uring_sqe_set_data(sqe, static_cast<UringReqBase*>(&req));
        io_uring_submit(ring);
    }), &req.task);
}

template <typename Req, typename F>
void UringReactorImpl::submitReq(Req& req, F prep, u64 deadlineUs) noexcept {
    auto ring = currentRing();
    auto now = monotonicNowUs();
    auto ts = (now < deadlineUs) ? usToTimespec(deadlineUs - now) : usToTimespec(0);

    exec_->parkWith(makeRunable([&] {
        auto sqe = io_uring_get_sqe(ring);

        prep(sqe);

        io_uring_sqe_set_data(sqe, static_cast<UringReqBase*>(&req));
        sqe->flags |= IOSQE_IO_LINK;

        auto tsqe = io_uring_get_sqe(ring);

        io_uring_prep_link_timeout(tsqe, &ts, 0);
        io_uring_sqe_set_data64(tsqe, WAKEUP_COOKIE);
        tsqe->flags |= IOSQE_CQE_SKIP_SUCCESS;
        io_uring_submit(ring);
    }), &req.task);
}

CondVarIface* UringReactorImpl::createCondVar(size_t index) {
    return new UringCondVarImpl(rings_[index], this);
}

int UringReactorImpl::recv(int fd, size_t* nRead, void* buf, size_t len, u64 deadlineUs) {
    RecvReq req;

    submit(req, [&](auto sqe) {
        io_uring_prep_recv(sqe, fd, buf, len, 0);
    }, deadlineUs);

    if (auto e = toErrno(req.res)) {
        return e;
    }

    *nRead = req.res;

    return 0;
}

int UringReactorImpl::send(int fd, size_t* nWritten, const void* buf, size_t len, u64 deadlineUs) {
    SendReq req;

    submit(req, [&](auto sqe) {
        io_uring_prep_send(sqe, fd, buf, len, MSG_NOSIGNAL);
    }, deadlineUs);

    if (auto e = toErrno(req.res)) {
        return e;
    }

    *nWritten = req.res;

    return 0;
}

int UringReactorImpl::writev(int fd, size_t* nWritten, iovec* iov, size_t iovcnt, u64 deadlineUs) {
    WritevReq req;

    submit(req, [&](auto sqe) {
        io_uring_prep_writev(sqe, fd, iov, iovcnt, 0);
    }, deadlineUs);

    if (auto e = toErrno(req.res)) {
        return e;
    }

    *nWritten = req.res;

    return 0;
}

int UringReactorImpl::accept(int fd, int* newFd, sockaddr* addr, u32* addrLen, u64 deadlineUs) {
    AcceptReq req;

    submit(req, [&](auto sqe) {
        io_uring_prep_accept(sqe, fd, addr, (socklen_t*)addrLen, SOCK_NONBLOCK | SOCK_CLOEXEC);
    }, deadlineUs);

    if (auto e = toErrno(req.res)) {
        return e;
    }

    *newFd = req.res;

    return 0;
}

int UringReactorImpl::connect(int fd, const sockaddr* addr, u32 addrLen, u64 deadlineUs) {
    ConnectReq req;

    submit(req, [&](auto sqe) {
        io_uring_prep_connect(sqe, fd, addr, addrLen);
    }, deadlineUs);

    return toErrno(req.res);
}

int UringReactorImpl::pread(int fd, size_t* nRead, void* buf, size_t len, off_t offset) {
    PreadReq req;

    submitReq(req, [&](auto sqe) {
        io_uring_prep_read(sqe, fd, buf, len, offset);
    });

    if (auto e = toErrno(req.res)) {
        return e;
    }

    *nRead = req.res;

    return 0;
}

int UringReactorImpl::pwrite(int fd, size_t* nWritten, const void* buf, size_t len, off_t offset) {
    PwriteReq req;

    submitReq(req, [&](auto sqe) {
        io_uring_prep_write(sqe, fd, buf, len, offset);
    });

    if (auto e = toErrno(req.res)) {
        return e;
    }

    *nWritten = req.res;

    return 0;
}

int UringReactorImpl::fsync(int fd) {
    FsyncReq req;

    submitReq(req, [&](auto sqe) {
        io_uring_prep_fsync(sqe, fd, 0);
    });

    return toErrno(req.res);
}

int UringReactorImpl::fdatasync(int fd) {
    FsyncReq req;

    submitReq(req, [&](auto sqe) {
        io_uring_prep_fsync(sqe, fd, IORING_FSYNC_DATASYNC);
    });

    return toErrno(req.res);
}

u32 UringReactorImpl::poll(PollFD pfd, u64 deadlineUs) {
    PollReq req;

    submit(req, [&](auto sqe) {
        io_uring_prep_poll_add(sqe, pfd.fd, toPollMask(pfd.flags));
    }, deadlineUs);

    if (req.res <= 0) {
        return 0;
    }

    return fromPollMask(req.res);
}

void UringReactorImpl::poll(PollGroup* g, VisitorFace& visitor, u64 deadlineUs) {
    auto impl = (UringPollGroup*)g;

    for (;;) {
        for (size_t i = 0; i < impl->count_; ++i) {
            auto res = poll(impl->fds_[i], monotonicNowUs() + 10000);

            if (res) {
                PollFD pfd = {impl->fds_[i].fd, res};

                visitor.visit(&pfd);

                return;
            }
        }

        if (monotonicNowUs() >= deadlineUs) {
            return;
        }
    }
}

PollGroup* UringReactorImpl::createPollGroup(ObjPool* pool, const PollFD* fds, size_t count) {
    return pool->make<UringPollGroup>(pool, fds, count);
}

void UringReactorImpl::sleep(u64 deadlineUs) {
    TimeoutReq req;

    req.ts = usToTimespec(deadlineUs);

    submitReq(req, [&](auto sqe) {
        io_uring_prep_timeout(sqe, &req.ts, 0, IORING_TIMEOUT_ABS);
    });
}

// UringPollGroup

UringPollGroup::UringPollGroup(ObjPool* pool, const PollFD* fds, size_t count)
    : fds_((PollFD*)pool->allocate(sizeof(PollFD) * count))
    , count_(count)
{
    for (size_t i = 0; i < count; ++i) {
        fds_[i] = fds[i];
    }
}

// factory

IoReactor* stl::createIoUringReactor(ObjPool* pool, CoroExecutor* exec, size_t threads) {
    try {
        return pool->make<UringReactorImpl>(pool, exec, threads);
    } catch (int) {
        return nullptr;
    }
}
#else
IoReactor* stl::createIoUringReactor(ObjPool*, CoroExecutor*, size_t) {
    return nullptr;
}
#endif
