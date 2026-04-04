#include "parker.h"

#include <std/sys/atomic.h>

using namespace stl;

Parker::Parker()
    : sleeping_(false)
{
}

Parker::~Parker() noexcept {
}

int Parker::fd() const noexcept {
    return ev_.fd();
}

void Parker::drain() noexcept {
    ev_.drain();
}

void Parker::unpark() noexcept {
    bool expected = true;

    if (stdAtomicCAS(&sleeping_, &expected, false, MemoryOrder::Acquire, MemoryOrder::Relaxed)) {
        ev_.signal();
    }
}

void Parker::signal() noexcept {
    ev_.signal();
}

void Parker::parkEnter() noexcept {
    stdAtomicStore(&sleeping_, true, MemoryOrder::Release);
}

void Parker::parkLeave() noexcept {
    stdAtomicStore(&sleeping_, false, MemoryOrder::Relaxed);
}
