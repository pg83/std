#include "string.h"

#include <std/str/dynamic.h>

using namespace Std;

StringOutput::~StringOutput() {
}

void StringOutput::writeImpl(const void* ptr, size_t len) {
    str_->append((const u8*)ptr, len);
}

size_t StringOutput::imbueImpl(void** ptr) noexcept {
    *ptr = str_->mutEnd();

    return str_->left();
}

void* StringOutput::imbueImpl(size_t len) {
    str_->growDelta(len);

    return str_->mutEnd();
}

void StringOutput::bumpImpl(const void* ptr) noexcept {
    str_->markInitialized(ptr);
}
