#include "io_reactor.h"
#include "poll_fd.h"

#include <std/lib/visitor.h>

using namespace stl;

void IoReactor::poll(PollGroup* g, VisitorFace&& visitor, u64 deadlineUs) {
    poll(g, visitor, deadlineUs);
}
