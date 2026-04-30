#include <std/thr/coro.h>
#include <std/lib/vector.h>
#include <std/thr/channel.h>
#include <std/mem/obj_pool.h>

using namespace stl;

namespace {
    void dutchRudder(int nStages, int nMessages) {
        auto mpool = ObjPool::fromMemory();
        auto opool = ObjPool::fromHugePages(mpool.mutPtr());
        auto exec = CoroExecutor::create(opool, 8);

        Vector<Channel*> chArr;

        for (int i = 0; i <= nStages; ++i) {
            chArr.pushBack(Channel::create(opool, exec, (size_t)5));
        }

        for (int i = 0; i < nStages; ++i) {
            exec->spawn([in = chArr[i], out = chArr[i + 1]] {
                void* batch[8];

                for (;;) {
                    size_t n = in->dequeue(batch, 8);

                    if (!n) {
                        break;
                    }

                    out->enqueue(batch, n);
                }

                out->close();
            });
        }

        i64 sum = 0;

        exec->spawn([ch = chArr[nStages], &sum] {
            void* batch[8];

            for (;;) {
                size_t n = ch->dequeue(batch, 8);

                if (!n) {
                    break;
                }

                for (size_t j = 0; j < n; ++j) {
                    sum += (i64)(uintptr_t)batch[j];
                }
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
