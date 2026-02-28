#include "h_table.h"

#include <std/alg/xchg.h>
#include <std/alg/minmax.h>
#include <std/dbg/assert.h>
#include <std/lib/vector.h>
#include <std/mem/obj_list.h>
#include <std/alg/exchange.h>

using namespace Std;

namespace {
    struct Node {
        void* value;
        Node* next;
        u64 key;
    };

    static inline size_t hash(u64 key, size_t capacity) noexcept {
        return key % capacity;
    }
}

struct HashTable::Impl {
    ObjList<Node> np;
    Vector<Node*> buckets;
    size_t count;

    inline Impl(size_t initialCapacity)
        : buckets(initialCapacity)
        , count(0)
    {
        for (size_t i = 0; i < initialCapacity; ++i) {
            buckets.pushBack(nullptr);
        }
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

    void* setNoRehash(u64 key, void* value) {
        if (auto node = findNode(key); node) {
            return exchange(node->value, value);
        }

        auto b = bucketFor(key);

        *b = np.make(Node{
            .value = value,
            .next = *b,
            .key = key,
        });

        ++count;

        return nullptr;
    }

    void* erase(u64 key) noexcept {
        for (auto ptr = bucketFor(key); *ptr; ptr = &(*ptr)->next) {
            auto node = *ptr;

            if (node->key == key) {
                void* value = node->value;

                *ptr = node->next;
                np.release(node);
                --count;

                return value;
            }
        }

        return nullptr;
    }

    template <typename T>
    inline void visit(T t) {
        for (size_t i = 0; i < buckets.length(); ++i) {
            for (auto node = buckets[i]; node; node = node->next) {
                t(node);
            }
        }
    }
};

HashTable::HashTable(size_t initialCapacity)
    : impl(new Impl(initialCapacity))
{
}

HashTable::~HashTable() noexcept {
    delete impl;
}

void HashTable::xchg(HashTable& t) noexcept {
    Std::xchg(impl, t.impl);
}

size_t HashTable::size() const noexcept {
    return impl->count;
}

void* HashTable::find(u64 key) const noexcept {
    if (auto node = impl->findNode(key); node) {
        return node->value;
    }

    return nullptr;
}

size_t HashTable::capacity() const noexcept {
    return impl->buckets.length();
}

void HashTable::rehash(size_t len) {
    HashTable next(len);

    impl->visit([&](auto node) {
        next.setNoRehash(node->key, node->value);
    });

    next.xchg(*this);
}

void* HashTable::setNoRehash(u64 key, void* value) {
    return impl->setNoRehash(key, value);
}

void* HashTable::set(u64 key, void* value) {
    if (size() >= capacity() * 0.7) {
        rehash(capacity() * 1.5);
    }

    return setNoRehash(key, value);
}

void* HashTable::erase(u64 key) noexcept {
    return impl->erase(key);
}

void HashTable::visitImpl(VisitorFace&& v) {
    impl->visit([&](auto node) {
        v.visit(&node->value);
    });
}
