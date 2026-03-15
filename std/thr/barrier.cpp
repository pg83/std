#include "barrier.h"

using namespace stl;

Barrier::Barrier(size_t n)
    : wg(n)
{
}

Barrier::Barrier(size_t n, CoroExecutor* exec)
    : wg(n, exec)
{
}

Barrier::~Barrier() noexcept {
}

void Barrier::wait() noexcept {
    wg.done();
    wg.wait();
}
