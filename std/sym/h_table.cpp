#include "h_table.h"

#include <std/alg/xchg.h>
#include <std/alg/minmax.h>
#include <std/alg/exchange.h>
#include <std/dbg/assert.h>
#include <std/lib/vector.h>
#include <std/mem/obj_list.h>

using namespace Std;

namespace {
    struct Node {
        u64 hash;
        void* value;
        Node* next;
    };

    static inline size_t hash(u64 key, size_t capacity) noexcept {
        return key % capacity;
    }
}

struct HashTable::Impl {
    Vector<Node*> buckets;
    ObjList<Node> nodePool;
    size_t count;

    inline Impl(size_t initialCapacity)
        : buckets(initialCapacity)
        , count(0)
    {
        for (size_t i = 0; i < initialCapacity; ++i) {
            buckets.pushBack(nullptr);
        }
    }

    Node* findNode(u64 key) const noexcept {
        size_t idx = hash(key, buckets.length());

        for (Node* node = buckets[idx]; node; node = node->next) {
            if (node->hash == key) {
                return node;
            }
        }

        return nullptr;
    }

    void* find(u64 key) const noexcept {
        auto node = findNode(key);

        return node ? node->value : nullptr;
    }

    void* set(u64 key, void* value) {
        auto node = findNode(key);

        if (node) {
            return exchange(node->value, value);
        }

        size_t idx = hash(key, buckets.length());
        Node* newNode = nodePool.make();

        newNode->hash = key;
        newNode->value = value;
        newNode->next = buckets.mut(idx);
        buckets.mut(idx) = newNode;

        ++count;

        return nullptr;
    }

    void* erase(u64 key) noexcept {
        size_t idx = hash(key, buckets.length());
        Node** nodePtr = &buckets.mut(idx);

        while (*nodePtr) {
            Node* node = *nodePtr;

            if (node->hash == key) {
                void* value = node->value;

                *nodePtr = node->next;
                nodePool.release(node);

                --count;

                return value;
            }

            nodePtr = &node->next;
        }

        return nullptr;
    }

    void rehash(size_t newCapacity) {
        Vector<Node*> oldBuckets;

        Std::xchg(oldBuckets, buckets);

        for (size_t i = 0; i < newCapacity; ++i) {
            buckets.pushBack(nullptr);
        }

        for (size_t i = 0; i < oldBuckets.length(); ++i) {
            for (Node* node = oldBuckets[i], *next; node; node = next) {
                next = node->next;
                size_t idx = hash(node->hash, buckets.length());
                node->next = exchange(buckets.mut(idx), node);
            }
        }
    }

    void visit(VisitorFace& v) {
        for (size_t i = 0; i < buckets.length(); ++i) {
            for (Node* node = buckets[i]; node; node = node->next) {
                v.visit(&node->value);
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
    return impl->find(key);
}

size_t HashTable::capacity() const noexcept {
    return impl->buckets.length();
}

void* HashTable::set(u64 key, void* value) {
    void* result = impl->set(key, value);

    if (impl->count >= impl->buckets.length() * 0.7) {
        impl->rehash(impl->buckets.length() * 1.5);
    }

    return result;
}

void* HashTable::erase(u64 key) noexcept {
    return impl->erase(key);
}

void HashTable::visitImpl(VisitorFace&& v) {
    impl->visit(v);
}
