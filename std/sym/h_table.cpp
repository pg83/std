#include "h_table.h"

#include <std/sys/crt.h>

#include <std/alg/xchg.h>
#include <std/alg/range.h>
#include <std/alg/minmax.h>

using namespace Std;

namespace {
    struct Entry {
        u64 key;
        void* value;

        inline bool filled() const noexcept {
            return value;
        }

        inline void erase() noexcept {
            value = (void*)1;
        }

        inline bool erased() const noexcept {
            return value == (void*)1;
        }

        inline bool used() const noexcept {
            return ((size_t)value) > 1;
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
    // 1.5 * 0.7 == 1.05 > 1
    rehashImpl(erange(buf).length() * 1.5);
}

void HashTable::rehashImpl(size_t initial) {
    HashTable next(initial);

    for (const auto& c : erange(buf)) {
        if (c.used()) {
            next.setNoRehash(c.key, c.value);
        }
    }

    next.xchg(*this);
}

HashTable::HashTable(size_t initialCapacity)
    : buf(max(initialCapacity, (size_t)1) * sizeof(Entry))
{
    memZero(buf.mutData(), buf.mutStorageEnd());
}

HashTable::~HashTable() noexcept {
}

void* HashTable::findEntryPtr(u64 key) const noexcept {
    auto r = erange(buf);
    auto c = r.length();

    for (auto i = hash(key, c);; i = (i + 1) % c) {
        const auto& el = r.b[i];

        if (!el.filled()) {
            return nullptr;
        }

        if (el.key == key) {
            if (el.erased()) {
                return nullptr;
            }

            return (void*)&el;
        }
    }

    return nullptr;
}

void HashTable::erase(u64 key) noexcept {
    if (auto el = findEntryPtr(key); el) {
        ((Entry*)el)->erase();
        buf.seekNegative(1);
    }
}

void* HashTable::find(u64 key) const noexcept {
    if (auto el = findEntryPtr(key); el) {
        return ((const Entry*)el)->value;
    }

    return nullptr;
}

void HashTable::set(u64 key, void* value) {
    if (size() >= capacity() * 0.7) {
        rehash();
    }

    setNoRehash(key, value);
}

void HashTable::setNoRehash(u64 key, void* value) {
    auto r = erange(buf);
    auto c = r.length();

    for (auto i = hash(key, c);; i = (i + 1) % c) {
        auto& el = r.b[i];

        if (!el.used()) {
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
        if (el.used()) {
            it.process(&el.value);
        }
    }
}

void HashTable::compactify() {
    rehashImpl(capacity());
}
