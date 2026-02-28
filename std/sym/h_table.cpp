#include "h_table.h"

#include <std/alg/xchg.h>
#include <std/alg/minmax.h>
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
        Node* node = buckets[idx];
        while (node) {
            if (node->hash == key) {
                return node;
            }
            node = node->next;
        }
        return nullptr;
    }

    void* find(u64 key) const noexcept {
        Node* node = findNode(key);
        return node ? node->value : nullptr;
    }

    void* set(u64 key, void* value) {
        Node* node = findNode(key);

        if (node) {
            void* oldValue = node->value;
            node->value = value;
            return oldValue;
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

        oldBuckets.xchg(buckets);

        for (size_t i = 0; i < newCapacity; ++i) {
            buckets.pushBack(nullptr);
        }

        for (size_t i = 0; i < oldBuckets.length(); ++i) {
            Node* node = oldBuckets[i];
            while (node) {
                Node* next = node->next;
                size_t idx = hash(node->hash, buckets.length());
                node->next = buckets.mut(idx);
                buckets.mut(idx) = node;
                node = next;
            }
        }
    }

    void visit(VisitorFace& v) {
        for (size_t i = 0; i < buckets.length(); ++i) {
            Node* node = buckets[i];
            while (node) {
                v.visit(&node->value);
                node = node->next;
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
