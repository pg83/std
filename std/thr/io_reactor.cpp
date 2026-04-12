#include "io_reactor.h"
#include "io_uring.h"
#include "io_classic.h"

#include <stdlib.h>

using namespace stl;

IoReactor* IoReactor::create(ObjPool* pool, CoroExecutor* exec, size_t threads) {
    if (getenv("USE_POLL_POLLER")) {
        return createPollIoReactor(pool, exec, threads);
    }

    if (auto io = createIoUringReactor(pool, exec, threads); io) {
        return io;
    } else {
        return createPollIoReactor(pool, exec, threads);
    }
}
