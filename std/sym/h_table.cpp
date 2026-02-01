#include "h_table.h"

#include <std/sys/crt.h>

#include <std/alg/xchg.h>
#include <std/alg/range.h>

using namespace Std;

namespace {
    struct Entry {
        u64 key;
        void* value;

        inline bool filled() const noexcept {
            return value;
        }
    };

    static inline auto erange(Buffer& b) noexcept {
        return range((Entry*)b.mutData(), (Entry*)b.mutStorageEnd());
    }

    static inline auto erange(const Buffer& b) noexcept {
        return range((const Entry*)b.data(), (const Entry*)b.storageEnd());
    }

    static inline size_t hash(u64 key, size_t capacity) noexcept {
        return key % capacity;
    }
}

void HashTable::rehash() {
    auto r = erange(buf);

    HashTable next(r.length() * 1.5);

    for (const auto& c : r) {
        if (c.filled()) {
            next.set(c.key, c.value);
        }
    }

    next.xchg(*this);
}

HashTable::HashTable(size_t initialCapacity)
    : buf(initialCapacity * sizeof(Entry))
{
    memZero(buf.mutData(), buf.mutStorageEnd());
}

HashTable::~HashTable() noexcept {
}

void* HashTable::find(u64 key) const noexcept {
    auto r = erange(buf);
    auto c = r.length();

    for (auto i = hash(key, c);; i = (i + 1) % c) {
        const auto& el = r.b[i];

        if (!el.filled()) {
            return nullptr;
        }

        if (el.key == key) {
            return el.value;
        }
    }

    return nullptr;
}

void HashTable::set(u64 key, void* value) {
    if (size() >= capacity() * 0.7) {
        rehash();
    }

    auto r = erange(buf);
    auto c = r.length();

    for (auto i = hash(key, c);; i = (i + 1) % c) {
        auto& el = r.b[i];

        if (!el.filled()) {
            el.key = key;
            el.value = value;

            buf.seekRelative(1);

            return;
        }

        if (el.key == key) {
            el.value = value;

            return;
        }
    }
}

size_t HashTable::capacity() const noexcept {
    return erange(buf).length();
}

void HashTable::forEach(Iterator& it) {
    for (auto& el : erange(buf)) {
        if (el.filled()) {
            it.process(&el.value);
        }
    }
}
