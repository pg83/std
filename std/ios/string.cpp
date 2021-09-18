#include "string.h"

#include <std/str/dynamic.h>

using namespace Std;

StringOutput::~StringOutput() {
}

void StringOutput::writeImpl(const void* ptr, size_t len) {
    str_->append((const c8*)ptr, len);
}
