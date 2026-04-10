#include <std/sys/crt.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <liburing.h>

static constexpr unsigned long long COOKIE = 0xCAFE;

struct Ring {
    struct io_uring ring;

    void initSimple() {
        if (io_uring_queue_init(64, &ring, 0) < 0) {
            printf("simple init failed\n");
            exit(1);
        }
    }

    void initDeferred() {
        struct io_uring_params params = {};

        params.flags = IORING_SETUP_SINGLE_ISSUER
                     | IORING_SETUP_DEFER_TASKRUN
                     | IORING_SETUP_R_DISABLED;

        if (io_uring_queue_init_params(64, &ring, &params) < 0) {
            printf("deferred init failed\n");
            exit(1);
        }
    }

    void enable() {
        io_uring_enable_rings(&ring);
    }

    void sendMsg(int targetFd) {
        auto sqe = io_uring_get_sqe(&ring);

        io_uring_prep_msg_ring(sqe, targetFd, 0, COOKIE, 0);

        int r = io_uring_submit(&ring);

        printf("sendMsg: from fd=%d to fd=%d, submit r=%d\n", ring.ring_fd, targetFd, r);
    }

    void waitForCookie() {
        for (;;) {
            io_uring_submit_and_wait(&ring, 1);

            struct io_uring_cqe* cqe;

            while (io_uring_peek_cqe(&ring, &cqe) == 0) {
                auto ud = cqe->user_data;
                auto res = cqe->res;

                io_uring_cqe_seen(&ring, cqe);

                printf("  cqe: res=%d ud=%llu\n", res, (unsigned long long)ud);

                if (ud == COOKIE) {
                    return;
                }
            }
        }
    }

    void destroy() {
        io_uring_queue_exit(&ring);
    }
};

static Ring ext;
static Ring worker;

static void* workerThread(void*) {
    worker.enable();

    printf("worker: enabled ring fd=%d, waiting...\n", worker.ring.ring_fd);

    worker.waitForCookie();

    printf("worker: got cookie!\n");

    return nullptr;
}

int main() {
    ext.initSimple();
    worker.initDeferred();

    printf("ext fd=%d, worker fd=%d\n", ext.ring.ring_fd, worker.ring.ring_fd);

    printf("\n--- Test: ext -> deferred worker (R_DISABLED + enable from worker thread) ---\n");

    pthread_t th;

    pthread_create(&th, nullptr, workerThread, nullptr);
    usleep(50000);

    ext.sendMsg(worker.ring.ring_fd);
    pthread_join(th, nullptr);

    printf("\nOK\n");

    ext.destroy();
    worker.destroy();

    return 0;
}
