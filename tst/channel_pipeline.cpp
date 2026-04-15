#include <std/thr/coro.h>
#include <std/lib/vector.h>
#include <std/thr/channel.h>
#include <std/mem/obj_pool.h>

using namespace stl;

namespace {
    void dutchRudder(int nStages, int nMessages) {
        auto opool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(opool.mutPtr(), 8);

        Vector<Channel*> chArr;

        for (int i = 0; i <= nStages; ++i) {
            chArr.pushBack(Channel::create(opool.mutPtr(), exec, (size_t)5));
        }

        for (int i = 0; i < nStages; ++i) {
            exec->spawn([in = chArr[i], out = chArr[i + 1]] {
                void* v;
                while (in->dequeue(&v)) {
                    out->enqueue(v);
                }
                out->close();
            });
        }

        i64 sum = 0;

        exec->spawn([ch = chArr[nStages], &sum] {
            void* v;
            while (ch->dequeue(&v)) {
                sum += (i64)(uintptr_t)v;
            }
        });

        exec->spawn([ch = chArr[0], nMessages] {
            for (int i = 1; i <= nMessages; ++i) {
                ch->enqueue((void*)(uintptr_t)i);
            }
            ch->close();
        });

        exec->join();
    }
}

int main() {
    dutchRudder(2000, 200000);
}
