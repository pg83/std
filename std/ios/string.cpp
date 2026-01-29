#include "string.h"

#include <std/str/dynamic.h>

using namespace Std;

StringOutput::~StringOutput() {
}

void StringOutput::writeImpl(const void* ptr, size_t len) {
    str_->append((const u8*)ptr, len);
}

void* StringOutput::imbueImpl(size_t len) {
    str_->growDelta(len);

    return str_->mutEnd();
}

void StringOutput::bumpImpl(const void* ptr) noexcept {
    str_->markInitialized(ptr);
}

size_t StringOutput::hintImpl() const noexcept {
    return size_t(-1);
}
