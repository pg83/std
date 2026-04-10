#include "io_uring.h"
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

using namespace stl;

#if __has_include(<liburing.h>)
namespace {
    constexpr u64 WAKEUP_COOKIE = 0xCAFE;

    struct Ring;

    thread_local Ring* currentRing = nullptr;

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

        UringReactorImpl(ObjPool* pool, size_t threads);

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

        struct io_uring_cqe* cqe;

        while (io_uring_peek_cqe(ring_, &cqe) == 0) {
            auto ud = cqe->user_data;

            io_uring_cqe_seen(ring_, cqe);

            if (ud == WAKEUP_COOKIE) {
                goto done;
            }
        }
    }

done:
    mutex.lock();
}

void UringCondVarImpl::signal() noexcept {
    currentRing()->wakeUp(ring_->ring_fd);
}

void UringCondVarImpl::broadcast() noexcept {
    signal();
}

UringReactorImpl::UringReactorImpl(ObjPool* pool, size_t threads)
    : ext_(pool->make<ExternalRing>())
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

int UringReactorImpl::recv(int, size_t*, void*, size_t, u64) {
    abort();
}

int UringReactorImpl::send(int, size_t*, const void*, size_t, u64) {
    abort();
}

int UringReactorImpl::writev(int, size_t*, iovec*, size_t, u64) {
    abort();
}

int UringReactorImpl::accept(int, int*, sockaddr*, u32*, u64) {
    abort();
}

int UringReactorImpl::connect(int, const sockaddr*, u32, u64) {
    abort();
}

int UringReactorImpl::pread(int, size_t*, void*, size_t, off_t) {
    abort();
}

int UringReactorImpl::pwrite(int, size_t*, const void*, size_t, off_t) {
    abort();
}

int UringReactorImpl::fsync(int) {
    abort();
}

int UringReactorImpl::fdatasync(int) {
    abort();
}

u32 UringReactorImpl::poll(PollFD, u64) {
    abort();
}

void UringReactorImpl::poll(PollGroup*, VisitorFace&, u64) {
    abort();
}

PollGroup* UringReactorImpl::createPollGroup(ObjPool*, const PollFD*, size_t) {
    abort();
}

void UringReactorImpl::sleep(u64) {
    abort();
}

// factory

IoReactor* stl::createIoUringReactor(ObjPool* pool, size_t threads) {
    try {
        return pool->make<UringReactorImpl>(pool, threads);
    } catch (int) {
        return nullptr;
    }
}
#else
IoReactor* stl::createIoUringReactor(ObjPool*, size_t) {
    return nullptr;
}
#endif
