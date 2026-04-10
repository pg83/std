#include "io_reactor.h"
#include "io_uring.h"
#include "io_classic.h"
#include "poll_fd.h"
#include "cond_var.h"

#include <std/lib/visitor.h>

using namespace stl;

CondVarIface* IoReactor::createCondVar(size_t) {
    return CondVar::createDefault();
}

void IoReactor::bindThread(size_t) {
}

void IoReactor::poll(PollGroup* g, VisitorFace&& visitor, u64 deadlineUs) {
    poll(g, visitor, deadlineUs);
}

IoReactor* IoReactor::create(ObjPool* pool, CoroExecutor* exec, size_t threads) {
    auto io = createIoUringReactor(pool, threads);

    if (!io) {
        io = createPollIoReactor(pool, exec, threads);
    }

    return io;
}
