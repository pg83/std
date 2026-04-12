#include "io_reactor.h"
#include "io_uring.h"
#include "io_classic.h"

using namespace stl;

IoReactor* IoReactor::create(ObjPool* pool, CoroExecutor* exec, size_t threads) {
    if (auto io = createIoUringReactor(pool, exec, threads); io) {
        return io;
    } else {
        return createPollIoReactor(pool, exec, threads);
    }
}
