#include "htable.h"

#include <std/sys/crt.h>
#include <std/alg/xchg.h>

using namespace Std;

namespace {
    static constexpr u64 emptyKey = 0;

    static inline size_t hash(u64 key, size_t capacity) noexcept {
        return key % capacity;
    }
}

void HashTable::rehash() {
    HashTable next(capacity * 2);

    for (size_t i = 0; i < capacity; ++i) {
        auto& item = table[i];

        if (item.key != emptyKey) {
            next.set(item.key, item.value);
        }
    }

    next.xchg(*this);
}

HashTable::HashTable(size_t initialCapacity)
    : capacity(initialCapacity)
    , size(0)
{
    table = static_cast<Entry*>(allocateZeroedMemory(capacity, sizeof(Entry)));
}

HashTable::~HashTable() noexcept {
    freeMemory(table);
}

void* HashTable::find(u64 key) const noexcept {
   size_t index = hash(key, capacity);
   size_t startIndex = index;

    do {
        if (table[index].key == emptyKey) {
            return nullptr;
        }

        if (table[index].key == key) {
            return table[index].value;
        }

        index = (index + 1) % capacity;
    } while (index != startIndex);

    return nullptr;
}

void HashTable::set(u64 key, void* value) {
    if (size >= capacity * 0.7) {
        rehash();
    }

    size_t index = hash(key, capacity);
    size_t startIndex = index;

    do {
        if (table[index].key == emptyKey) {
            table[index].key = key;
            table[index].value = value;
            ++size;
            return;
        }

        if (table[index].key == key) {
            table[index].value = value;
            return;
        }

        index = (index + 1) % capacity;
    } while (index != startIndex);

    rehash();

    set(key, value);
}

void HashTable::xchg(HashTable& t) noexcept {
    Std::xchg(size, t.size);
    Std::xchg(table, t.table);
    Std::xchg(capacity, t.capacity);
}
