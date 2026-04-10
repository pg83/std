#include <std/sys/fd.h>
#include <std/thr/coro.h>
#include <std/lib/vector.h>
#include <std/thr/poll_fd.h>
#include <std/mem/obj_pool.h>
#include <std/thr/io_reactor.h>

#include <errno.h>
#include <unistd.h>

using namespace stl;

int main() {
    auto opool = ObjPool::fromMemory();
    auto exec = CoroExecutor::create(opool.mutPtr(), 8);
    const int N = 200;
    const size_t TOTAL = 500 * 1024 * 1024;

    struct Pipe {
        ScopedFD r, w;
        Pipe() {
            createPipeFD(r, w);
            r.setNonBlocking();
            w.setNonBlocking();
        }
        ~Pipe() noexcept {
        }
    };

    Vector<Pipe*> pipes;

    for (int i = 0; i < N; i++) {
        auto p = opool->make<Pipe>();
        pipes.pushBack(p);

        int rfd = p->r.get();
        int wfd = p->w.get();

        CoroExecutor* ex = exec;

        exec->spawnRun(SpawnParams().setStackSize(64 * 1024).setRunable([ex, rfd] {
            char buf[16 * 1024];
            size_t received = 0;

            while (received < TOTAL) {
                ssize_t n = ::read(rfd, buf, sizeof(buf));

                if (n > 0) {
                    received += (size_t)n;
                } else if (errno == EAGAIN) {
                    ex->io()->poll({rfd, PollFlag::In}, UINT64_MAX);
                }
            }
        }));

        exec->spawnRun(SpawnParams().setStackSize(64 * 1024).setRunable([ex, wfd] {
            char buf[16 * 1024] = {};
            size_t sent = 0;

            while (sent < TOTAL) {
                size_t rem = TOTAL - sent;
                ssize_t n = ::write(wfd, buf, rem < sizeof(buf) ? rem : sizeof(buf));

                if (n > 0) {
                    sent += (size_t)n;
                } else if (errno == EAGAIN) {
                    ex->io()->poll({wfd, PollFlag::Out}, UINT64_MAX);
                }
            }
        }));
    }

    exec->join();
}
