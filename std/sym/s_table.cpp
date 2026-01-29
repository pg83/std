#include "s_table.h"

#include <std/str/view.h>

using namespace Std;

SymbolTable::SymbolTable() {
}

SymbolTable::~SymbolTable() noexcept {
}

void SymbolTable::set(const StringView& key, void* v) {
    htable.set(key.hash64(), v);
}

void* SymbolTable::find(const StringView& key) const noexcept {
    return htable.find(key.hash64());
}
