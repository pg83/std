#include "h_table.h"

#include <std/tst/ut.h>
#include <std/rng/pcg.h>
#include <std/lib/vector.h>

#include <string.h>

using namespace stl;

namespace {
    struct TestNode: public HashTable::Node {
        int value;
    };
}

STD_TEST_SUITE(HashTable) {
    STD_TEST(BasicInsertAndFind) {
        HashTable ht;

        TestNode node1 = {{1, nullptr}, 100};
        TestNode node2 = {{2, nullptr}, 200};
        TestNode node3 = {{3, nullptr}, 300};

        ht.insert(&node1);
        ht.insert(&node2);
        ht.insert(&node3);

        auto* found1 = static_cast<TestNode*>(ht.find(1));
        auto* found2 = static_cast<TestNode*>(ht.find(2));
        auto* found3 = static_cast<TestNode*>(ht.find(3));

        STD_INSIST(found1 == &node1 && found1->value == 100);
        STD_INSIST(found2 == &node2 && found2->value == 200);
        STD_INSIST(found3 == &node3 && found3->value == 300);
        STD_INSIST(ht.size() == 3);
    }

    STD_TEST(FindNonExistent) {
        HashTable ht;

        TestNode node = {{1, nullptr}, 42};
        ht.insert(&node);

        STD_INSIST(ht.find(999) == nullptr);
        STD_INSIST(ht.find(0) == nullptr);
        STD_INSIST(ht.find(2) == nullptr);
    }

    STD_TEST(UpdateExisting) {
        HashTable ht;

        TestNode node1 = {{1, nullptr}, 100};
        TestNode node2 = {{1, nullptr}, 200};

        auto* old = ht.insert(&node1);
        STD_INSIST(old == nullptr);
        STD_INSIST(ht.find(1) == &node1);
        STD_INSIST(ht.size() == 1);

        old = ht.insert(&node2);
        STD_INSIST(old == &node1);
        STD_INSIST(ht.find(1) == &node2);
        STD_INSIST(ht.size() == 1);
    }

    STD_TEST(LargeNumberOfElements) {
        HashTable ht;

        const size_t count = 10000;
        Vector<TestNode> nodes;
        nodes.grow(count);

        for (size_t i = 0; i < count; ++i) {
            nodes.mutData()[i] ={{i + 1, nullptr}, (int)(i * 7)};
        }

        for (size_t i = 0; i < count; ++i) {
            ht.insert(&nodes.mutData()[i]);
        }

        STD_INSIST(ht.size() == count);

        for (size_t i = 0; i < count; ++i) {
            auto* found = static_cast<TestNode*>(ht.find(i + 1));
            STD_INSIST(found != nullptr);
            STD_INSIST(found->value == i * 7);
        }
    }

    STD_TEST(RehashWorks) {
        HashTable ht(4);

        TestNode nodes[200];
        for (int i = 0; i < 200; ++i) {
            nodes[i] = {{(u64)(i + 1), nullptr}, i * 10};
        }

        size_t initialCapacity = ht.capacity();

        for (int i = 0; i < 200; ++i) {
            ht.insert(&nodes[i]);
        }

        STD_INSIST(ht.capacity() > initialCapacity);
        STD_INSIST(ht.size() == 200);

        for (int i = 0; i < 200; ++i) {
            auto* found = static_cast<TestNode*>(ht.find(i + 1));
            STD_INSIST(found != nullptr);
            STD_INSIST(found->value == i * 10);
        }
    }

    STD_TEST(CollisionHandling) {
        HashTable ht(16);

        TestNode nodes[5];
        for (int i = 0; i < 5; ++i) {
            nodes[i] = {{(u64)((i + 1) * 17), nullptr}, i};
        }

        ht.insert(&nodes[0]);
        ht.insert(&nodes[1]);
        ht.insert(&nodes[2]);
        ht.insert(&nodes[3]);
        ht.insert(&nodes[4]);

        STD_INSIST(ht.size() == 5);

        STD_INSIST(ht.find(17) == &nodes[0]);
        STD_INSIST(ht.find(34) == &nodes[1]);
        STD_INSIST(ht.find(51) == &nodes[2]);
        STD_INSIST(ht.find(68) == &nodes[3]);
        STD_INSIST(ht.find(85) == &nodes[4]);
    }

    STD_TEST(EraseBasic) {
        HashTable ht;

        TestNode node1 = {{1, nullptr}, 100};
        TestNode node2 = {{2, nullptr}, 200};
        TestNode node3 = {{3, nullptr}, 300};

        ht.insert(&node1);
        ht.insert(&node2);
        ht.insert(&node3);

        STD_INSIST(ht.size() == 3);

        auto* erased = ht.erase(2);

        STD_INSIST(erased == &node2);
        STD_INSIST(ht.find(1) == &node1);
        STD_INSIST(ht.find(2) == nullptr);
        STD_INSIST(ht.find(3) == &node3);

        STD_INSIST(ht.size() == 2);
    }

    STD_TEST(EraseNonExistent) {
        HashTable ht;

        TestNode node = {{1, nullptr}, 42};
        ht.insert(&node);

        STD_INSIST(ht.size() == 1);

        STD_INSIST(ht.erase(999) == nullptr);
        STD_INSIST(ht.erase(0) == nullptr);
        STD_INSIST(ht.erase(2) == nullptr);

        STD_INSIST(ht.size() == 1);
        STD_INSIST(ht.find(1) == &node);
    }

    STD_TEST(EraseAndReinsert) {
        HashTable ht;

        TestNode node1 = {{1, nullptr}, 100};
        TestNode node2 = {{1, nullptr}, 200};

        ht.insert(&node1);
        STD_INSIST(ht.find(1) == &node1);

        auto* erased = ht.erase(1);
        STD_INSIST(erased == &node1);
        STD_INSIST(ht.find(1) == nullptr);

        ht.insert(&node2);
        STD_INSIST(ht.find(1) == &node2);
    }

    STD_TEST(EraseAllElements) {
        HashTable ht;

        TestNode nodes[5];
        for (int i = 0; i < 5; ++i) {
            nodes[i] = {{(u64)(i + 1), nullptr}, (i + 1) * 10};
        }

        for (int i = 0; i < 5; ++i) {
            ht.insert(&nodes[i]);
        }

        STD_INSIST(ht.size() == 5);

        for (int i = 0; i < 5; ++i) {
            auto* erased = ht.erase(i + 1);
            STD_INSIST(erased == &nodes[i]);
        }

        for (int i = 0; i < 5; ++i) {
            STD_INSIST(ht.find(i + 1) == nullptr);
        }

        STD_INSIST(ht.size() == 0);
    }

    STD_TEST(VisitEmptyTable) {
        HashTable ht;

        int count = 0;
        ht.visit([&count](HashTable::Node* node) {
            count++;
        });

        STD_INSIST(count == 0);
    }

    STD_TEST(VisitSingleElement) {
        HashTable ht;
        TestNode node = {{1, nullptr}, 42};
        ht.insert(&node);

        int count = 0;
        HashTable::Node* lastNode = nullptr;
        ht.visit([&count, &lastNode](HashTable::Node* n) {
            count++;
            lastNode = n;
        });

        STD_INSIST(count == 1);
        STD_INSIST(lastNode == &node);
    }

    STD_TEST(VisitMultipleElements) {
        HashTable ht;

        TestNode nodes[5];
        for (int i = 0; i < 5; ++i) {
            nodes[i] = {{(u64)(i + 1), nullptr}, (i + 1) * 10};
            ht.insert(&nodes[i]);
        }

        int count = 0;
        ht.visit([&count](HashTable::Node* node) {
            count++;
        });

        STD_INSIST(count == 5);
        STD_INSIST(ht.size() == 5);
    }

    STD_TEST(VisitAllElements) {
        HashTable ht;

        TestNode nodes[10];
        for (int i = 0; i < 10; ++i) {
            nodes[i] = {{(u64)(i + 1), nullptr}, i * 10};
            ht.insert(&nodes[i]);
        }

        int sum = 0;
        ht.visit([&sum](HashTable::Node* node) {
            auto* tn = static_cast<TestNode*>(node);
            sum += tn->value;
        });

        STD_INSIST(sum == 0 + 10 + 20 + 30 + 40 + 50 + 60 + 70 + 80 + 90);
    }

    STD_TEST(VisitModifyValues) {
        HashTable ht;

        TestNode nodes[5];
        for (int i = 0; i < 5; ++i) {
            nodes[i] = {{(u64)(i + 1), nullptr}, (i + 1)};
            ht.insert(&nodes[i]);
        }

        ht.visit([](HashTable::Node* node) {
            auto* tn = static_cast<TestNode*>(node);
            tn->value *= 2;
        });

        for (int i = 0; i < 5; ++i) {
            auto* found = static_cast<TestNode*>(ht.find(i + 1));
            STD_INSIST(found != nullptr);
            STD_INSIST(found->value == (i + 1) * 2);
        }
    }

    STD_TEST(EraseWithCollisions) {
        HashTable ht(16);

        TestNode nodes[5];
        for (int i = 0; i < 5; ++i) {
            nodes[i] = {{(u64)((i + 1) * 17), nullptr}, i};
        }

        for (int i = 0; i < 5; ++i) {
            ht.insert(&nodes[i]);
        }

        auto* erased = ht.erase(51);
        STD_INSIST(erased == &nodes[2]);

        STD_INSIST(ht.find(17) == &nodes[0]);
        STD_INSIST(ht.find(34) == &nodes[1]);
        STD_INSIST(ht.find(51) == nullptr);
        STD_INSIST(ht.find(68) == &nodes[3]);
        STD_INSIST(ht.find(85) == &nodes[4]);
    }

    STD_TEST(EraseTwice) {
        HashTable ht;

        TestNode node = {{1, nullptr}, 42};
        ht.insert(&node);

        auto* erased = ht.erase(1);
        STD_INSIST(erased == &node);
        STD_INSIST(ht.find(1) == nullptr);

        erased = ht.erase(1);
        STD_INSIST(erased == nullptr);
        STD_INSIST(ht.find(1) == nullptr);
    }

    STD_TEST(EraseAfterUpdate) {
        HashTable ht;

        TestNode node1 = {{1, nullptr}, 100};
        TestNode node2 = {{1, nullptr}, 200};

        ht.insert(&node1);
        auto* old = ht.insert(&node2);
        STD_INSIST(old == &node1);

        STD_INSIST(ht.find(1) == &node2);

        auto* erased = ht.erase(1);
        STD_INSIST(erased == &node2);

        STD_INSIST(ht.find(1) == nullptr);
    }

    STD_TEST(_StressTestRandomOperations) {
        HashTable ht;
        PCG32 rng(0xDEADBEEF);

        const size_t iterations = 10000;
        const size_t keyRange = 1000;
        Vector<TestNode> nodes;
        nodes.grow(keyRange * 2);
        TestNode* expectedNodes[keyRange];
        bool shouldExist[keyRange];

        for (size_t i = 0; i < keyRange * 2; ++i) {
            nodes.mutData()[i] ={{0, nullptr}, (int)(i * 17)};
        }

        for (size_t i = 0; i < keyRange; ++i) {
            expectedNodes[i] = nullptr;
            shouldExist[i] = false;
        }

        size_t expectedSize = 0;

        for (size_t iter = 0; iter < iterations; ++iter) {
            u32 op = rng.uniformBiased(100);
            u64 key = rng.uniformBiased(keyRange) + 1;
            size_t idx = key - 1;

            if (op < 40) {
                nodes.mutData()[idx].key = key;
                auto prev = ht.insert(&nodes.mutData()[idx]);
                expectedNodes[idx] = &nodes.mutData()[idx];
                if (!shouldExist[idx]) {
                    STD_INSIST(!prev);
                    shouldExist[idx] = true;
                    expectedSize++;
                } else {
                    STD_INSIST(prev);
                }
            } else if (op < 70) {
                auto* result = ht.find(key);
                if (shouldExist[idx]) {
                    STD_INSIST(result == expectedNodes[idx]);
                } else {
                    STD_INSIST(result == nullptr);
                }
            } else if (op < 90) {
                ht.erase(key);
                if (shouldExist[idx]) {
                    shouldExist[idx] = false;
                    expectedNodes[idx] = nullptr;
                    expectedSize--;
                }
            } else {
                if (shouldExist[idx]) {
                    size_t newIdx = (idx + keyRange + iter) % (keyRange * 2);

                    if (nodes.mutData()[newIdx].key != 0) {
                        size_t oldIdx = nodes.mutData()[newIdx].key - 1;
                        if (shouldExist[oldIdx] && expectedNodes[oldIdx] == &nodes.mutData()[newIdx]) {
                            ht.erase(nodes.mutData()[newIdx].key);
                            shouldExist[oldIdx] = false;
                            expectedNodes[oldIdx] = nullptr;
                            expectedSize--;
                        }
                    }

                    nodes.mutData()[newIdx].key = key;
                    STD_INSIST(ht.insert(&nodes.mutData()[newIdx]));
                    expectedNodes[idx] = &nodes.mutData()[newIdx];
                }
            }

            size_t cnt = 0;
            ht.visit([&](auto x) {
                ++cnt;
            });
            STD_INSIST(ht.size() == expectedSize);
            STD_INSIST(cnt == expectedSize);
        }

        for (size_t i = 0; i < keyRange; ++i) {
            auto* result = ht.find(i + 1);
            if (shouldExist[i]) {
                STD_INSIST(result == expectedNodes[i]);
            } else {
                STD_INSIST(result == nullptr);
            }
        }
    }

    STD_TEST(StressTestGrowthAndShrinkage) {
        HashTable ht(4);

        const size_t maxSize = 2000;
        Vector<TestNode> nodes;
        nodes.grow(maxSize);

        for (size_t i = 0; i < maxSize; ++i) {
            nodes.mutData()[i] ={{i + 1, nullptr}, (int)i};
        }

        for (size_t phase = 0; phase < 10; ++phase) {
            for (size_t i = 0; i < maxSize; ++i) {
                ht.insert(&nodes.mutData()[i]);
            }

            STD_INSIST(ht.size() == maxSize);

            for (size_t i = 0; i < maxSize; i += 2) {
                ht.erase(i + 1);
            }

            STD_INSIST(ht.size() == maxSize / 2);

            for (size_t i = 0; i < maxSize; i += 2) {
                ht.erase(i + 1 + 1);
            }

            STD_INSIST(ht.size() == 0);
        }
    }

    STD_TEST(StressTestDeepCollisionChains) {
        HashTable ht(64);

        const size_t capacity = ht.capacity();
        const size_t chainLength = 30;
        Vector<TestNode> nodes;
        nodes.grow(chainLength);

        for (size_t i = 0; i < chainLength; ++i) {
            nodes.mutData()[i] ={{i * capacity, nullptr}, (int)i};
            ht.insert(&nodes.mutData()[i]);
        }

        for (size_t i = 0; i < chainLength; ++i) {
            u64 key = i * capacity;
            STD_INSIST(ht.find(key) == &nodes.mutData()[i]);
        }

        for (size_t i = 0; i < chainLength; i += 3) {
            u64 key = i * capacity;
            ht.erase(key);
        }

        for (size_t i = 0; i < chainLength; ++i) {
            u64 key = i * capacity;
            auto* result = ht.find(key);
            if (i % 3 == 0) {
                STD_INSIST(result == nullptr);
            } else {
                STD_INSIST(result == &nodes.mutData()[i]);
            }
        }

        for (size_t i = 0; i < chainLength; i += 3) {
            u64 key = i * capacity;
            ht.insert(&nodes.mutData()[i]);
        }

        for (size_t i = 0; i < chainLength; ++i) {
            u64 key = i * capacity;
            STD_INSIST(ht.find(key) == &nodes.mutData()[i]);
        }
    }

    STD_TEST(StressTestHeavyLoad) {
        HashTable ht;
        PCG32 rng(0xFEEDBEEF);

        const size_t keyRange = 2000;
        const size_t operations = 20000;
        Vector<TestNode> nodes;
        nodes.grow(keyRange);

        for (size_t i = 0; i < keyRange; ++i) {
            nodes.mutData()[i] ={{i + 1, nullptr}, (int)i};
        }

        for (size_t op = 0; op < operations; ++op) {
            u32 choice = rng.uniformBiased(100);
            u64 key = rng.uniformBiased(keyRange) + 1;
            size_t idx = key - 1;

            if (choice < 50) {
                ht.insert(&nodes.mutData()[idx]);
            } else if (choice < 80) {
                ht.find(key);
            } else if (choice < 95) {
                ht.erase(key);
            } else {
                ht.erase(key);
            }
        }

        for (size_t verify = 0; verify < 1000; ++verify) {
            u64 key = rng.uniformBiased(keyRange) + 1;
            ht.find(key);
        }
    }
}
