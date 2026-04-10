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

    thread_local struct io_uring* currentRing = nullptr;

    struct Ring: public io_uring {
        Ring();

        ~Ring() noexcept {
            io_uring_queue_exit(this);
        }

        void enable() noexcept;
    };

    struct ExternalRing: public io_uring {
        Mutex mutex_{true};

        ExternalRing();

        ~ExternalRing() noexcept {
            io_uring_queue_exit(this);
        }

        void sendMsg(int targetFd) noexcept;
    };

    struct UringCondVarImpl: public CondVarIface {
        Ring* ring_;
        ExternalRing* ext_;

        UringCondVarImpl(Ring* ring, ExternalRing* ext) noexcept
            : ring_(ring)
            , ext_(ext)
        {
        }

        void wait(Mutex& mutex) noexcept override;
        void signal() noexcept override;

        void broadcast() noexcept override {
            signal();
        }
    };

    struct UringReactorImpl: public IoReactor, public ThreadPoolHooks {
        Vector<Ring*> rings_;
        ExternalRing* ext_;

        UringReactorImpl(ObjPool* pool, size_t threads);

        ThreadPoolHooks* hooks() override {
            return this;
        }

        CondVarIface* createCondVar(size_t index) override {
            return new UringCondVarImpl(rings_[index], ext_);
        }

        void bindThread(size_t index) override {
            auto ring = rings_[index];

            ring->enable();
            currentRing = ring;
        }

        int recv(int, size_t*, void*, size_t, u64) override { abort(); }
        int send(int, size_t*, const void*, size_t, u64) override { abort(); }
        int writev(int, size_t*, iovec*, size_t, u64) override { abort(); }
        int accept(int, int*, sockaddr*, u32*, u64) override { abort(); }
        int connect(int, const sockaddr*, u32, u64) override { abort(); }
        int pread(int, size_t*, void*, size_t, off_t) override { abort(); }
        int pwrite(int, size_t*, const void*, size_t, off_t) override { abort(); }
        int fsync(int) override { abort(); }
        int fdatasync(int) override { abort(); }
        u32 poll(PollFD, u64) override { abort(); }
        void poll(PollGroup*, VisitorFace&, u64) override { abort(); }
        PollGroup* createPollGroup(ObjPool*, const PollFD*, size_t) override { abort(); }
        void sleep(u64) override { abort(); }
    };
}

Ring::Ring() {
    struct io_uring_params params = {};

    params.flags = IORING_SETUP_SINGLE_ISSUER
                 | IORING_SETUP_DEFER_TASKRUN
                 | IORING_SETUP_R_DISABLED;

    if (io_uring_queue_init_params(64, this, &params) < 0) {
        throw 1;
    }
}

void Ring::enable() noexcept {
    io_uring_enable_rings(this);
}

ExternalRing::ExternalRing() {
    if (io_uring_queue_init(64, this, 0) < 0) {
        throw 1;
    }

    mutex_.unlock();
}

void ExternalRing::sendMsg(int targetFd) noexcept {
    mutex_.lock();

    auto sqe = io_uring_get_sqe(this);

    io_uring_prep_msg_ring(sqe, targetFd, 0, WAKEUP_COOKIE, 0);
    io_uring_submit(this);

    mutex_.unlock();
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
    auto targetFd = ring_->ring_fd;

    if (auto myRing = currentRing; myRing) {
        auto sqe = io_uring_get_sqe(myRing);

        io_uring_prep_msg_ring(sqe, targetFd, 0, WAKEUP_COOKIE, 0);
        io_uring_submit(myRing);
    } else {
        ext_->sendMsg(targetFd);
    }
}

UringReactorImpl::UringReactorImpl(ObjPool* pool, size_t threads)
    : ext_(pool->make<ExternalRing>())
{
    for (size_t i = 0; i < threads; ++i) {
        rings_.pushBack(pool->make<Ring>());
    }
}

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
