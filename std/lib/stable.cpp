#include "stable.h"

#include <xxhash.h>

#include <std/str/view.h>

using namespace Std;

StringTable::StringTable() {
}

StringTable::~StringTable() noexcept {
}

void StringTable::set(const StringView& key, void* v) {
    htable.set(key.hash64(), v);
}

void* StringTable::find(const StringView& key) const noexcept {
    return htable.find(key.hash64());
}
