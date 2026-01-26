#include "htable.h"

#include <std/sys/crt.h>
#include <std/alg/xchg.h>

using namespace Std;

namespace {
    static inline size_t hash(u64 key, size_t capacity) noexcept {
        return key % capacity;
    }
}

void HashTable::rehash() {
    HashTable next(capacity * 2);

    for (size_t i = 0; i < capacity; ++i) {
        auto& item = table[i];

        if (item.filled()) {
            next.set(item.key, item.value);
        }
    }

    next.xchg(*this);
}

HashTable::HashTable(size_t initialCapacity)
    : capacity(initialCapacity)
    , size(0)
    , table((Entry*)allocateZeroedMemory(capacity, sizeof(Entry)))
{
}

HashTable::~HashTable() noexcept {
    freeMemory(table);
}

void* HashTable::find(u64 key) const noexcept {
    size_t index = hash(key, capacity);
    size_t startIndex = index;

    do {
        const auto& el = table[index];

        if (!el.filled()) {
            return nullptr;
        }

        if (el.key == key) {
            return el.value;
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

    while (true) {
        auto& el = table[index];

        if (!el.filled()) {
            el.key = key;
            el.value = value;
            ++size;
            return;
        }

        if (el.key == key) {
            el.value = value;
            return;
        }

        index = (index + 1) % capacity;
    }
}

void HashTable::xchg(HashTable& t) noexcept {
    Std::xchg(size, t.size);
    Std::xchg(table, t.table);
    Std::xchg(capacity, t.capacity);
}
