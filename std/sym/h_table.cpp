#include "h_table.h"

#include <std/sys/crt.h>
#include <std/alg/range.h>
#include <std/alg/minmax.h>
#include <std/alg/exchange.h>

using namespace stl;

namespace {
    static inline auto buckets(const Buffer& buf) noexcept {
        return range((HashTable::Node**)buf.data(), (HashTable::Node**)buf.storageEnd());
    }
}

HashTable::HashTable(size_t initialCapacity)
    : buf(max(initialCapacity, (size_t)1) * sizeof(Node*))
{
    // here we assume (2^N - 2 * sizeof(void*)) % sizeof(void*) == 0 for N > 3
    memZero(buf.mutData(), buf.mutStorageEnd());
}

HashTable::~HashTable() noexcept {
}

void HashTable::xchg(HashTable& t) noexcept {
    buf.xchg(t.buf);
}

size_t HashTable::size() const noexcept {
    return buf.used();
}

HashTable::Node* HashTable::find(u64 key) const noexcept {
    for (auto node = *bucketFor(key); node; node = node->next) {
        if (node->key == key) {
            return node;
        }
    }

    return nullptr;
}

size_t HashTable::capacity() const noexcept {
    return buckets(buf).length();
}

void HashTable::rehash(size_t len) {
    HashTable next(len);

    visit([&](auto node) {
        next.addNoRehash(node);
    });

    next.xchg(*this);
}

HashTable::Node* HashTable::insert(Node* nn) {
    if (auto cap = capacity(); size() >= cap * 0.7) {
        rehash(cap * 1.5);
    }

    auto res = erase(nn->key);

    return (addNoRehash(nn), res);
}

HashTable::Node* HashTable::erase(u64 key) noexcept {
    for (auto ptr = bucketFor(key); *ptr; ptr = &(*ptr)->next) {
        if (auto node = *ptr; node->key == key) {
            *ptr = node->next;
            buf.seekNegative(1);

            return node;
        }
    }

    return nullptr;
}

HashTable::Node** HashTable::bucketFor(u64 key) const noexcept {
    auto bb = buckets(buf);

    return &bb.b[key % bb.length()];
}

void HashTable::addNoRehash(Node* nn) {
    auto b = bucketFor(nn->key);

    nn->next = *b;
    *b = nn;

    buf.seekRelative(1);
}

void HashTable::visitImpl(VisitorFace&& v) {
    for (auto node : buckets(buf)) {
        while (node) {
            v.visit(exchange(node, node->next));
        }
    }
}
