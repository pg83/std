#include "io_uring.h"

#include "coro.h"
#include "pool.h"
#include "mutex.h"
#include "poller.h"
#include "poll_fd.h"
#include "io_reactor.h"
#include "cond_var.h"
#include "cond_var_iface.h"

#include <std/sys/crt.h>
#include <std/lib/list.h>
#include <std/lib/vector.h>
#include <std/lib/visitor.h>
#include <std/mem/obj_pool.h>

#include <errno.h>
#include <stdlib.h>
#include <sys/socket.h>

#if __has_include(<liburing.h>)
    #include <liburing.h>
#endif

using namespace stl;

#if __has_include(<liburing.h>)
namespace {
    constexpr u64 WAKEUP_COOKIE = 0xCAFE;

    struct Ring;

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

    struct UringReq: public UringReqBase {
        Task* task;
        int res;

        void complete(int result, IntrusiveList& ready) noexcept override;

        int error() noexcept;

        template <typename T>
        int readInto(T* out) noexcept;
    };

    struct TimeoutReq: public UringReq {
        __kernel_timespec ts;
    };

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

        io_uring_sqe* getSqe() noexcept;
        void getSqe(io_uring_sqe*& a, io_uring_sqe*& b) noexcept;

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

        bool cycle() noexcept;

        void signal() noexcept override;
        void broadcast() noexcept override;
        void wait(Mutex& mutex) noexcept override;
    };

    struct UringReactorImpl: public IoReactor, public ThreadPoolHooks {
        Vector<Ring*> rings_;
        Ring* ext_;
        CoroExecutor* exec_;

        Ring* currentRing() noexcept;

        template <typename F>
        UringReq submit(F prep, u64 deadlineUs) noexcept;

        template <typename Req, typename F>
        void submitReq(Req& req, F prep) noexcept;

        template <typename Req, typename F>
        void submitReq(Req& req, F prep, u64 deadlineUs) noexcept;

        UringReactorImpl(ObjPool* pool, CoroExecutor* exec, size_t threads);

        ThreadPoolHooks* hooks() override;
        Mutex* createMutex(ObjPool* pool) override;
        CondVar* createCondVar(ObjPool* pool, size_t index) override;

        int recv(int, size_t*, void*, size_t, u64) override;
        int recvfrom(int, size_t*, void*, size_t, sockaddr*, u32*, u64) override;
        int send(int, size_t*, const void*, size_t, u64) override;
        int sendto(int, size_t*, const void*, size_t, const sockaddr*, u32, u64) override;
        int writev(int, size_t*, iovec*, size_t, u64) override;
        int accept(int, int*, sockaddr*, u32*, u64) override;
        int connect(int, const sockaddr*, u32, u64) override;
        int pread(int, size_t*, void*, size_t, off_t) override;
        int pwrite(int, size_t*, const void*, size_t, off_t) override;
        int fsync(int) override;
        int fdatasync(int) override;
        u32 poll(PollFD, u64) override;
        PollerIface* createPoller(ObjPool*) override;
        void sleep(u64) override;
    };
}

void UringReq::complete(int result, IntrusiveList& ready) noexcept {
    res = result;
    ready.pushBack(task);
}

int UringReq::error() noexcept {
    return toErrno(res);
}

template <typename T>
int UringReq::readInto(T* out) noexcept {
    if (auto e = toErrno(res)) {
        return e;
    }

    *out = res;

    return 0;
}

Ring::Ring()
    : Ring(IORING_SETUP_SINGLE_ISSUER | IORING_SETUP_DEFER_TASKRUN | IORING_SETUP_R_DISABLED)
{
}

Ring::Ring(u32 flags) {
    struct io_uring_params params = {};

    params.flags = flags;

    if (io_uring_queue_init_params(256, this, &params) < 0) {
        throw 1;
    }
}

Ring::~Ring() noexcept {
    io_uring_queue_exit(this);
}

io_uring_sqe* Ring::getSqe() noexcept {
    auto sqe = io_uring_get_sqe(this);

    if (!sqe) {
        io_uring_submit(this);
        sqe = io_uring_get_sqe(this);
    }

    return sqe;
}

void Ring::getSqe(io_uring_sqe*& a, io_uring_sqe*& b) noexcept {
    a = io_uring_get_sqe(this);
    b = io_uring_get_sqe(this);

    if (!b) {
        io_uring_submit(this);
        a = io_uring_get_sqe(this);
        b = io_uring_get_sqe(this);
    }
}

void Ring::enable() noexcept {
    io_uring_enable_rings(this);
}

void Ring::sendMsg(int targetFd) noexcept {
    auto sqe = getSqe();

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
    ring_->enable();
}

bool UringCondVarImpl::cycle() noexcept {
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
        reactor_->exec_->pool()->submitTasks(ready);
        reactor_->exec_->pool()->flushLocal();
    }

    return signaled;
}

void UringCondVarImpl::wait(Mutex& mutex) noexcept {
    mutex.unlock();

    while (!cycle()) {
    }

    mutex.lock();
}

void UringCondVarImpl::signal() noexcept {
    reactor_->currentRing()->wakeUp(ring_->ring_fd);
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
    size_t id;

    if (exec_->pool()->workerId(&id)) {
        return rings_[id];
    }

    return ext_;
}

template <typename F>
UringReq UringReactorImpl::submit(F prep, u64 deadlineUs) noexcept {
    UringReq req;

    if (deadlineUs == UINT64_MAX) {
        submitReq(req, prep);
    } else {
        submitReq(req, prep, deadlineUs);
    }

    return req;
}

template <typename Req, typename F>
void UringReactorImpl::submitReq(Req& req, F prep) noexcept {
    auto ring = currentRing();

    exec_->parkWith(makeRunable([&] {
        auto sqe = ring->getSqe();

        prep(sqe);

        io_uring_sqe_set_data(sqe, static_cast<UringReqBase*>(&req));
    }), &req.task);
}

template <typename Req, typename F>
void UringReactorImpl::submitReq(Req& req, F prep, u64 deadlineUs) noexcept {
    auto ring = currentRing();
    auto now = monotonicNowUs();
    auto ts = (now < deadlineUs) ? usToTimespec(deadlineUs - now) : usToTimespec(0);

    exec_->parkWith(makeRunable([&] {
        io_uring_sqe* sqe;
        io_uring_sqe* tsqe;

        ring->getSqe(sqe, tsqe);

        prep(sqe);

        io_uring_sqe_set_data(sqe, static_cast<UringReqBase*>(&req));
        sqe->flags |= IOSQE_IO_LINK;

        io_uring_prep_link_timeout(tsqe, &ts, 0);
        io_uring_sqe_set_data64(tsqe, WAKEUP_COOKIE);
        tsqe->flags |= IOSQE_CQE_SKIP_SUCCESS;
    }), &req.task);
}

Mutex* UringReactorImpl::createMutex(ObjPool* pool) {
    return pool->make<Mutex>(Mutex::spinLock(nullptr));
}

CondVar* UringReactorImpl::createCondVar(ObjPool* pool, size_t index) {
    return pool->make<CondVar>(new UringCondVarImpl(rings_[index], this));
}

int UringReactorImpl::recv(int fd, size_t* nRead, void* buf, size_t len, u64 deadlineUs) {
    return submit([&](auto sqe) {
        io_uring_prep_recv(sqe, fd, buf, len, 0);
    }, deadlineUs).readInto(nRead);
}

int UringReactorImpl::recvfrom(int fd, size_t* nRead, void* buf, size_t len, sockaddr* addr, u32* addrLen, u64 deadlineUs) {
    struct msghdr msg = {};
    struct iovec iov = {buf, len};

    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_name = addr;
    msg.msg_namelen = addrLen ? *addrLen : 0;

    auto res = submit([&](auto sqe) {
        io_uring_prep_recvmsg(sqe, fd, &msg, 0);
    }, deadlineUs).readInto(nRead);

    if (!res && addrLen) {
        *addrLen = msg.msg_namelen;
    }

    return res;
}

int UringReactorImpl::send(int fd, size_t* nWritten, const void* buf, size_t len, u64 deadlineUs) {
    return submit([&](auto sqe) {
        io_uring_prep_send(sqe, fd, buf, len, MSG_NOSIGNAL);
    }, deadlineUs).readInto(nWritten);
}

int UringReactorImpl::sendto(int fd, size_t* nWritten, const void* buf, size_t len, const sockaddr* addr, u32 addrLen, u64 deadlineUs) {
    struct msghdr msg = {};
    struct iovec iov = {const_cast<void*>(buf), len};

    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_name = const_cast<sockaddr*>(addr);
    msg.msg_namelen = addrLen;

    return submit([&](auto sqe) {
        io_uring_prep_sendmsg(sqe, fd, &msg, MSG_NOSIGNAL);
    }, deadlineUs).readInto(nWritten);
}

int UringReactorImpl::writev(int fd, size_t* nWritten, iovec* iov, size_t iovcnt, u64 deadlineUs) {
    return submit([&](auto sqe) {
        io_uring_prep_writev(sqe, fd, iov, iovcnt, 0);
    }, deadlineUs).readInto(nWritten);
}

int UringReactorImpl::accept(int fd, int* newFd, sockaddr* addr, u32* addrLen, u64 deadlineUs) {
    return submit([&](auto sqe) {
        io_uring_prep_accept(sqe, fd, addr, (socklen_t*)addrLen, SOCK_NONBLOCK | SOCK_CLOEXEC);
    }, deadlineUs).readInto(newFd);
}

int UringReactorImpl::connect(int fd, const sockaddr* addr, u32 addrLen, u64 deadlineUs) {
    return submit([&](auto sqe) {
        io_uring_prep_connect(sqe, fd, addr, addrLen);
    }, deadlineUs).error();
}

int UringReactorImpl::pread(int fd, size_t* nRead, void* buf, size_t len, off_t offset) {
    return submit([&](auto sqe) {
        io_uring_prep_read(sqe, fd, buf, len, offset);
    }, UINT64_MAX).readInto(nRead);
}

int UringReactorImpl::pwrite(int fd, size_t* nWritten, const void* buf, size_t len, off_t offset) {
    return submit([&](auto sqe) {
        io_uring_prep_write(sqe, fd, buf, len, offset);
    }, UINT64_MAX).readInto(nWritten);
}

int UringReactorImpl::fsync(int fd) {
    return submit([&](auto sqe) {
        io_uring_prep_fsync(sqe, fd, 0);
    }, UINT64_MAX).error();
}

int UringReactorImpl::fdatasync(int fd) {
    return submit([&](auto sqe) {
        io_uring_prep_fsync(sqe, fd, IORING_FSYNC_DATASYNC);
    }, UINT64_MAX).error();
}

u32 UringReactorImpl::poll(PollFD pfd, u64 deadlineUs) {
    auto req = submit([&](auto sqe) {
        io_uring_prep_poll_add(sqe, pfd.fd, pfd.toPollEvents());
    }, deadlineUs);

    if (req.res <= 0) {
        return 0;
    }

    return PollFD::fromPollEvents(req.res);
}

PollerIface* UringReactorImpl::createPoller(ObjPool* pool) {
    return WaitablePoller::create(pool, this);
}

void UringReactorImpl::sleep(u64 deadlineUs) {
    TimeoutReq req;

    req.ts = usToTimespec(deadlineUs);

    submitReq(req, [&](auto sqe) {
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
