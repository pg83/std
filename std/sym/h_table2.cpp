#include "h_table2.h"

#include <std/alg/xchg.h>
#include <std/alg/minmax.h>
#include <std/dbg/assert.h>
#include <std/lib/vector.h>
#include <std/alg/exchange.h>

using namespace Std;

namespace {
    static inline size_t hash(u64 key, size_t capacity) noexcept {
        return key % capacity;
    }
}

struct HashTable2::Impl {
    Vector<Node*> buckets;
    size_t count;

    inline Impl(size_t initialCapacity)
        : count(0)
    {
        buckets.zero(max(initialCapacity, (size_t)1));
    }

    inline auto bucketFor(u64 key) const noexcept {
        return (Node**)&buckets[hash(key, buckets.length())];
    }

    inline Node* findNode(u64 key) const noexcept {
        for (auto node = *bucketFor(key); node; node = node->next) {
            if (node->key == key) {
                return node;
            }
        }

        return nullptr;
    }

    inline Node* insertNoRehash(Node* newNode) {
        auto res = erase(newNode->key);

        return (addNoRehash(newNode), res);
    }

    inline void addNoRehash(Node* newNode) {
        auto b = bucketFor(newNode->key);

        newNode->next = *b;
        *b = newNode;

        ++count;
    }

    inline Node* erase(u64 key) noexcept {
        for (auto ptr = bucketFor(key); *ptr; ptr = &(*ptr)->next) {
            auto node = *ptr;

            if (node->key == key) {
                *ptr = node->next;
                --count;

                return node;
            }
        }

        return nullptr;
    }

    template <typename T>
    inline void visit(T t) {
        for (size_t i = 0; i < buckets.length(); ++i) {
            for (auto node = buckets[i]; node;) {
                t(exchange(node, node->next));
            }
        }
    }
};

HashTable2::HashTable2(size_t initialCapacity)
    : impl(new Impl(initialCapacity))
{
}

HashTable2::~HashTable2() noexcept {
    delete impl;
}

void HashTable2::xchg(HashTable2& t) noexcept {
    Std::xchg(impl, t.impl);
}

size_t HashTable2::size() const noexcept {
    return impl->count;
}

HashTable2::Node* HashTable2::find(u64 key) const noexcept {
    return impl->findNode(key);
}

size_t HashTable2::capacity() const noexcept {
    return impl->buckets.length();
}

void HashTable2::rehash(size_t len) {
    HashTable2 next(len);

    impl->visit([&](auto node) {
        next.impl->addNoRehash(node);
    });

    next.xchg(*this);
}

HashTable2::Node* HashTable2::insert(Node* newNode) {
    if (size() >= capacity() * 0.7) {
        rehash(capacity() * 1.5);
    }

    return impl->insertNoRehash(newNode);
}

HashTable2::Node* HashTable2::erase(u64 key) noexcept {
    return impl->erase(key);
}

void HashTable2::visitImpl(VisitorFace&& v) {
    impl->visit([&](auto node) {
        v.visit(node);
    });
}
