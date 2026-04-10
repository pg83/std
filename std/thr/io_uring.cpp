#include "io_uring.h"
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
    thread_local struct io_uring* currentRing = nullptr;

    struct UringCondVarImpl: public CondVarIface {
        struct io_uring* ring_;
        int ringFd_;

        UringCondVarImpl(struct io_uring* ring) noexcept
            : ring_(ring)
            , ringFd_(ring->ring_fd)
        {
        }

        void wait(Mutex& mutex) noexcept override {
            mutex.unlock();
            io_uring_submit_and_wait(ring_, 1);

            struct io_uring_cqe* cqe;

            while (io_uring_peek_cqe(ring_, &cqe) == 0) {
                io_uring_cqe_seen(ring_, cqe);
            }

            mutex.lock();
        }

        void signal() noexcept override {
            auto myRing = currentRing;
            auto sqe = io_uring_get_sqe(myRing);

            io_uring_prep_msg_ring(sqe, ringFd_, 0, 0, 0);
            io_uring_submit(myRing);
        }

        void broadcast() noexcept override {
            signal();
        }
    };

    struct UringReactorImpl: public IoReactor {
        Vector<struct io_uring> rings_;

        UringReactorImpl(size_t threads) {
            for (size_t i = 0; i < threads; ++i) {
                struct io_uring ring;
                struct io_uring_params params = {};

                params.flags = IORING_SETUP_SINGLE_ISSUER | IORING_SETUP_DEFER_TASKRUN;

                if (io_uring_queue_init_params(64, &ring, &params) < 0) {
                    for (size_t j = 0; j < rings_.length(); ++j) {
                        io_uring_queue_exit(&rings_[j]);
                    }

                    throw this;
                }

                rings_.pushBack(ring);
            }
        }

        ~UringReactorImpl() noexcept {
            for (size_t i = 0; i < rings_.length(); ++i) {
                io_uring_queue_exit(&rings_[i]);
            }
        }

        CondVarIface* createCondVar(size_t index) override {
            return new UringCondVarImpl(&rings_[index]);
        }

        void bindThread(size_t index) override {
            currentRing = &rings_[index];
        }

        int recv(int, size_t*, void*, size_t, u64) override {
            abort();
        }

        int send(int, size_t*, const void*, size_t, u64) override {
            abort();
        }

        int writev(int, size_t*, iovec*, size_t, u64) override {
            abort();
        }

        int accept(int, int*, sockaddr*, u32*, u64) override {
            abort();
        }

        int connect(int, const sockaddr*, u32, u64) override {
            abort();
        }

        int pread(int, size_t*, void*, size_t, off_t) override {
            abort();
        }

        int pwrite(int, size_t*, const void*, size_t, off_t) override {
            abort();
        }

        int fsync(int) override {
            abort();
        }

        int fdatasync(int) override {
            abort();
        }

        u32 poll(PollFD, u64) override {
            abort();
        }

        void poll(PollGroup*, VisitorFace&, u64) override {
            abort();
        }

        PollGroup* createPollGroup(ObjPool*, const PollFD*, size_t) override {
            abort();
        }

        void sleep(u64) override {
            abort();
        }
    };
}

IoReactor* stl::createIoUringReactor(ObjPool* pool, size_t threads) {
    try {
        return pool->make<UringReactorImpl>(threads);
    } catch (UringReactorImpl*) {
        return nullptr;
    }
}
#else
IoReactor* stl::createIoUringReactor(ObjPool*, size_t) {
    return nullptr;
}
#endif
