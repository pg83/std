#include <std/thr/coro.h>
#include <std/sys/atomic.h>
#include <std/mem/obj_pool.h>

using namespace stl;

namespace {
    void doW(int work) {
        for (volatile int i = 0; i < work; i = i + 1) {
        }
    }
}

int main() {
    const int depth = 22;
    const int work = 999;

    auto pool = ObjPool::fromMemory();
    auto exec = CoroExecutor::create(pool.mutPtr(), 16);

    int counter2 = 0;

    auto run = [&](auto& self, int d) -> void {
        stdAtomicAddAndFetch(&counter2, 1, MemoryOrder::Relaxed);

        doW(work);

        if (d > 0) {
            exec->spawnRun(SpawnParams().setStackSize(2000).setRunable([&, d] {
                self(self, d - 1);
            }));
        }

        exec->yield();

        if (d > 0) {
            exec->spawnRun(SpawnParams().setStackSize(2000).setRunable([&, d] {
                self(self, d - 1);
            }));
        }

        doW(work);
    };

    exec->spawn([&] {
        run(run, depth);
    });

    exec->join();
}
