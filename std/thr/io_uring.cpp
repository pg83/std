#include "io_uring.h"
#include "coro.h"
#include "pool.h"
#include "mutex.h"
#include "poll_fd.h"
#include "io_reactor.h"
#include "cond_var_iface.h"

#include <std/sys/crt.h>
#include <std/lib/vector.h>
#include <std/mem/obj_pool.h>

#if __has_include(<liburing.h>)
    #include <liburing.h>
#endif

#include <stdlib.h>
#include <poll.h>
#include <sys/socket.h>

using namespace stl;

#if __has_include(<liburing.h>)
namespace {
    constexpr u64 WAKEUP_COOKIE = 0xCAFE;

    struct Ring;

    thread_local Ring* currentRing = nullptr;

    struct UringReq {
        Task* task;
        int res;
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

        virtual ~Ring() noexcept;

        void enable() noexcept;
        void sendMsg(int targetFd) noexcept;

        virtual void wakeUp(int targetFd) noexcept;
    };

    struct ExternalRing: public Ring {
        Mutex mutex_{true};

        ExternalRing();

        void wakeUp(int targetFd) noexcept override;
    };

    template <typename Req, typename F>
    void submitReq(CoroExecutor* exec, Req& req, F prep) noexcept {
        auto ring = currentRing;

        exec->parkWith(makeRunable([&] {
            auto sqe = io_uring_get_sqe(ring);

            prep(sqe);
            io_uring_sqe_set_data(sqe, static_cast<UringReq*>(&req));
        }), &req.task);
    }

    struct UringReactorImpl;

    struct UringCondVarImpl: public CondVarIface {
        Ring* ring_;
        UringReactorImpl* reactor_;

        UringCondVarImpl(Ring* ring, UringReactorImpl* reactor) noexcept;

        Ring* currentRing() noexcept;

        void signal() noexcept override;
        void broadcast() noexcept override;
        void wait(Mutex& mutex) noexcept override;
    };

    struct UringReactorImpl: public IoReactor, public ThreadPoolHooks {
        Vector<Ring*> rings_;
        Ring* ext_;
        CoroExecutor* exec_;

        UringReactorImpl(ObjPool* pool, CoroExecutor* exec, size_t threads);

        ThreadPoolHooks* hooks() override;
        CondVarIface* createCondVar(size_t index) override;
        void bindThread(size_t index) override;

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

Ring::Ring() {
    struct io_uring_params params = {};

    params.flags = IORING_SETUP_SINGLE_ISSUER | IORING_SETUP_DEFER_TASKRUN | IORING_SETUP_R_DISABLED;

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
    io_uring_submit(this);
}

void Ring::wakeUp(int targetFd) noexcept {
    sendMsg(targetFd);
}

ExternalRing::ExternalRing() {
    io_uring_queue_exit(this);

    if (io_uring_queue_init(64, this, 0) < 0) {
        throw 1;
    }

    mutex_.unlock();
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

Ring* UringCondVarImpl::currentRing() noexcept {
    if (auto r = ::currentRing; r) {
        return r;
    }

    return reactor_->ext_;
}

void UringCondVarImpl::wait(Mutex& mutex) noexcept {
    mutex.unlock();

    for (;;) {
        io_uring_submit_and_wait(ring_, 1);

        bool hasWork = false;
        struct io_uring_cqe* cqe;

        while (io_uring_peek_cqe(ring_, &cqe) == 0) {
            auto ud = cqe->user_data;
            auto res = cqe->res;

            io_uring_cqe_seen(ring_, cqe);

            if (ud == WAKEUP_COOKIE) {
                hasWork = true;
            } else {
                auto req = (UringReq*)ud;

                req->res = res;
                reactor_->exec_->reSchedule(req->task);
                hasWork = true;
            }
        }

        if (hasWork) {
            break;
        }
    }

    mutex.lock();
}

void UringCondVarImpl::signal() noexcept {
    currentRing()->wakeUp(ring_->ring_fd);
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

CondVarIface* UringReactorImpl::createCondVar(size_t index) {
    return new UringCondVarImpl(rings_[index], this);
}

void UringReactorImpl::bindThread(size_t index) {
    (currentRing = rings_[index])->enable();
}

int UringReactorImpl::recv(int fd, size_t* nRead, void* buf, size_t len, u64) {
    RecvReq req;

    submitReq(exec_, req, [&](auto sqe) {
        io_uring_prep_recv(sqe, fd, buf, len, 0);
    });

    if (req.res < 0) {
        return -req.res;
    }

    *nRead = req.res;

    return 0;
}

int UringReactorImpl::send(int fd, size_t* nWritten, const void* buf, size_t len, u64) {
    SendReq req;

    submitReq(exec_, req, [&](auto sqe) {
        io_uring_prep_send(sqe, fd, buf, len, MSG_NOSIGNAL);
    });

    if (req.res < 0) {
        return -req.res;
    }

    *nWritten = req.res;

    return 0;
}

int UringReactorImpl::writev(int fd, size_t* nWritten, iovec* iov, size_t iovcnt, u64) {
    WritevReq req;

    submitReq(exec_, req, [&](auto sqe) {
        io_uring_prep_writev(sqe, fd, iov, iovcnt, 0);
    });

    if (req.res < 0) {
        return -req.res;
    }

    *nWritten = req.res;

    return 0;
}

int UringReactorImpl::accept(int fd, int* newFd, sockaddr* addr, u32* addrLen, u64) {
    AcceptReq req;

    submitReq(exec_, req, [&](auto sqe) {
        io_uring_prep_accept(sqe, fd, addr, (socklen_t*)addrLen, SOCK_NONBLOCK | SOCK_CLOEXEC);
    });

    if (req.res < 0) {
        return -req.res;
    }

    *newFd = req.res;

    return 0;
}

int UringReactorImpl::connect(int fd, const sockaddr* addr, u32 addrLen, u64) {
    ConnectReq req;

    submitReq(exec_, req, [&](auto sqe) {
        io_uring_prep_connect(sqe, fd, addr, addrLen);
    });

    if (req.res < 0) {
        return -req.res;
    }

    return 0;
}

int UringReactorImpl::pread(int fd, size_t* nRead, void* buf, size_t len, off_t offset) {
    PreadReq req;

    submitReq(exec_, req, [&](auto sqe) {
        io_uring_prep_read(sqe, fd, buf, len, offset);
    });

    if (req.res < 0) {
        return -req.res;
    }

    *nRead = req.res;

    return 0;
}

int UringReactorImpl::pwrite(int fd, size_t* nWritten, const void* buf, size_t len, off_t offset) {
    PwriteReq req;

    submitReq(exec_, req, [&](auto sqe) {
        io_uring_prep_write(sqe, fd, buf, len, offset);
    });

    if (req.res < 0) {
        return -req.res;
    }

    *nWritten = req.res;

    return 0;
}

int UringReactorImpl::fsync(int fd) {
    FsyncReq req;

    submitReq(exec_, req, [&](auto sqe) {
        io_uring_prep_fsync(sqe, fd, 0);
    });

    return req.res < 0 ? -req.res : 0;
}

int UringReactorImpl::fdatasync(int fd) {
    FsyncReq req;

    submitReq(exec_, req, [&](auto sqe) {
        io_uring_prep_fsync(sqe, fd, IORING_FSYNC_DATASYNC);
    });

    return req.res < 0 ? -req.res : 0;
}

u32 UringReactorImpl::poll(PollFD pfd, u64) {
    PollReq req;

    submitReq(exec_, req, [&](auto sqe) {
        io_uring_prep_poll_add(sqe, pfd.fd, toPollMask(pfd.flags));
    });

    return fromPollMask(req.res);
}

void UringReactorImpl::poll(PollGroup*, VisitorFace&, u64) {
    abort();
}

PollGroup* UringReactorImpl::createPollGroup(ObjPool*, const PollFD*, size_t) {
    abort();
}

void UringReactorImpl::sleep(u64 deadlineUs) {
    TimeoutReq req;

    req.ts = usToTimespec(deadlineUs);

    submitReq(exec_, req, [&](auto sqe) {
        io_uring_prep_timeout(sqe, &req.ts, 0, IORING_TIMEOUT_ABS);
    });
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
