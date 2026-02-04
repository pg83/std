#include "len.h"

#include <std/str/view.h>
#include <std/mem/scratch.h>

#include <sys/uio.h>

using namespace Std;

CountingOutput::CountingOutput() noexcept
    : len_(0)
{
}

size_t CountingOutput::writeImpl(const void*, size_t len) {
    len_ += len;

    return len;
}

void* CountingOutput::imbueImpl(size_t len) {
    if (len <= 1024) {
        // TODO(pg): check cache line bouncing friendliness
        return scratchMem();
    }

    buf_.grow(len);

    return buf_.mutData();
}

size_t CountingOutput::hintImpl() const noexcept {
    return 1024;
}

void CountingOutput::bumpImpl(const void* ptr) noexcept {
    auto sm = (const u8*)scratchMem();
    auto pm = (const u8*)ptr;

    if (sm <= pm && pm <= sm + 1024) {
        len_ += pm - sm;
    } else {
        len_ += buf_.offsetOf(ptr);
    }
}
