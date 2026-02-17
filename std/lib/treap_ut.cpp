#include "treap.h"

#include <std/tst/ut.h>

using namespace Std;

STD_TEST_SUITE(Treap) {
    struct IntTreapNode: public TreapNode {
        int value;

        IntTreapNode(int v)
            : value(v)
        {
        }

        void* key() override {
            return &value;
        }
    };

    class IntTreap: public Treap {
    public:
        bool cmp(void* a, void* b) override {
            int* ia = static_cast<int*>(a);
            int* ib = static_cast<int*>(b);
            return *ia < *ib;
        }
    };

    STD_TEST(EmptyTreap) {
        IntTreap treap;
        int key = 42;
        STD_INSIST(treap.find(&key) == nullptr);
    }

    STD_TEST(InsertSingleElement) {
        IntTreap treap;
        IntTreapNode node(10);
        treap.insert(&node);

        int key = 10;
        TreapNode* found = treap.find(&key);
        STD_INSIST(found != nullptr);
        STD_INSIST(static_cast<IntTreapNode*>(found)->value == 10);
    }

    STD_TEST(InsertMultipleElements) {
        IntTreap treap;
        IntTreapNode node1(5);
        IntTreapNode node2(10);
        IntTreapNode node3(15);

        treap.insert(&node1);
        treap.insert(&node2);
        treap.insert(&node3);

        int key1 = 5;
        TreapNode* found1 = treap.find(&key1);
        STD_INSIST(found1 != nullptr);
        STD_INSIST(static_cast<IntTreapNode*>(found1)->value == 5);

        int key2 = 10;
        TreapNode* found2 = treap.find(&key2);
        STD_INSIST(found2 != nullptr);
        STD_INSIST(static_cast<IntTreapNode*>(found2)->value == 10);

        int key3 = 15;
        TreapNode* found3 = treap.find(&key3);
        STD_INSIST(found3 != nullptr);
        STD_INSIST(static_cast<IntTreapNode*>(found3)->value == 15);
    }

    STD_TEST(FindNonExistentKey) {
        IntTreap treap;
        IntTreapNode node1(10);
        IntTreapNode node2(20);
        IntTreapNode node3(30);

        treap.insert(&node1);
        treap.insert(&node2);
        treap.insert(&node3);

        int key = 15;
        TreapNode* found = treap.find(&key);
        STD_INSIST(found == nullptr);
    }

    STD_TEST(InsertAscendingOrder) {
        IntTreap treap;
        IntTreapNode node1(1);
        IntTreapNode node2(2);
        IntTreapNode node3(3);
        IntTreapNode node4(4);
        IntTreapNode node5(5);

        treap.insert(&node1);
        treap.insert(&node2);
        treap.insert(&node3);
        treap.insert(&node4);
        treap.insert(&node5);

        for (int i = 1; i <= 5; i++) {
            TreapNode* found = treap.find(&i);
            STD_INSIST(found != nullptr);
            STD_INSIST(static_cast<IntTreapNode*>(found)->value == i);
        }
    }

    STD_TEST(InsertDescendingOrder) {
        IntTreap treap;
        IntTreapNode node1(5);
        IntTreapNode node2(4);
        IntTreapNode node3(3);
        IntTreapNode node4(2);
        IntTreapNode node5(1);

        treap.insert(&node1);
        treap.insert(&node2);
        treap.insert(&node3);
        treap.insert(&node4);
        treap.insert(&node5);

        for (int i = 1; i <= 5; i++) {
            TreapNode* found = treap.find(&i);
            STD_INSIST(found != nullptr);
            STD_INSIST(static_cast<IntTreapNode*>(found)->value == i);
        }
    }

    STD_TEST(InsertRandomOrder) {
        IntTreap treap;
        IntTreapNode node1(15);
        IntTreapNode node2(5);
        IntTreapNode node3(25);
        IntTreapNode node4(3);
        IntTreapNode node5(20);
        IntTreapNode node6(30);
        IntTreapNode node7(10);

        treap.insert(&node1);
        treap.insert(&node2);
        treap.insert(&node3);
        treap.insert(&node4);
        treap.insert(&node5);
        treap.insert(&node6);
        treap.insert(&node7);

        int keys[] = {3, 5, 10, 15, 20, 25, 30};
        for (int key : keys) {
            TreapNode* found = treap.find(&key);
            STD_INSIST(found != nullptr);
            STD_INSIST(static_cast<IntTreapNode*>(found)->value == key);
        }
    }

    STD_TEST(FindBoundaryKeys) {
        IntTreap treap;
        IntTreapNode node1(10);
        IntTreapNode node2(20);
        IntTreapNode node3(30);

        treap.insert(&node1);
        treap.insert(&node2);
        treap.insert(&node3);

        int key1 = 5;
        STD_INSIST(treap.find(&key1) == nullptr);

        int key2 = 35;
        STD_INSIST(treap.find(&key2) == nullptr);

        int key3 = 10;
        STD_INSIST(treap.find(&key3) != nullptr);

        int key4 = 30;
        STD_INSIST(treap.find(&key4) != nullptr);
    }

    STD_TEST(InsertDuplicateValues) {
        IntTreap treap;
        IntTreapNode node1(10);
        IntTreapNode node2(10);
        IntTreapNode node3(10);

        treap.insert(&node1);
        treap.insert(&node2);
        treap.insert(&node3);

        int key = 10;
        TreapNode* found = treap.find(&key);
        STD_INSIST(found != nullptr);
        STD_INSIST(static_cast<IntTreapNode*>(found)->value == 10);
    }

    STD_TEST(InsertManyElements) {
        IntTreap treap;
        IntTreapNode nodes[100] = {
            IntTreapNode(0), IntTreapNode(1), IntTreapNode(2), IntTreapNode(3), IntTreapNode(4),
            IntTreapNode(5), IntTreapNode(6), IntTreapNode(7), IntTreapNode(8), IntTreapNode(9),
            IntTreapNode(10), IntTreapNode(11), IntTreapNode(12), IntTreapNode(13), IntTreapNode(14),
            IntTreapNode(15), IntTreapNode(16), IntTreapNode(17), IntTreapNode(18), IntTreapNode(19),
            IntTreapNode(20), IntTreapNode(21), IntTreapNode(22), IntTreapNode(23), IntTreapNode(24),
            IntTreapNode(25), IntTreapNode(26), IntTreapNode(27), IntTreapNode(28), IntTreapNode(29),
            IntTreapNode(30), IntTreapNode(31), IntTreapNode(32), IntTreapNode(33), IntTreapNode(34),
            IntTreapNode(35), IntTreapNode(36), IntTreapNode(37), IntTreapNode(38), IntTreapNode(39),
            IntTreapNode(40), IntTreapNode(41), IntTreapNode(42), IntTreapNode(43), IntTreapNode(44),
            IntTreapNode(45), IntTreapNode(46), IntTreapNode(47), IntTreapNode(48), IntTreapNode(49),
            IntTreapNode(50), IntTreapNode(51), IntTreapNode(52), IntTreapNode(53), IntTreapNode(54),
            IntTreapNode(55), IntTreapNode(56), IntTreapNode(57), IntTreapNode(58), IntTreapNode(59),
            IntTreapNode(60), IntTreapNode(61), IntTreapNode(62), IntTreapNode(63), IntTreapNode(64),
            IntTreapNode(65), IntTreapNode(66), IntTreapNode(67), IntTreapNode(68), IntTreapNode(69),
            IntTreapNode(70), IntTreapNode(71), IntTreapNode(72), IntTreapNode(73), IntTreapNode(74),
            IntTreapNode(75), IntTreapNode(76), IntTreapNode(77), IntTreapNode(78), IntTreapNode(79),
            IntTreapNode(80), IntTreapNode(81), IntTreapNode(82), IntTreapNode(83), IntTreapNode(84),
            IntTreapNode(85), IntTreapNode(86), IntTreapNode(87), IntTreapNode(88), IntTreapNode(89),
            IntTreapNode(90), IntTreapNode(91), IntTreapNode(92), IntTreapNode(93), IntTreapNode(94),
            IntTreapNode(95), IntTreapNode(96), IntTreapNode(97), IntTreapNode(98), IntTreapNode(99)};

        for (int i = 0; i < 100; i++) {
            treap.insert(&nodes[i]);
        }

        for (int i = 0; i < 100; i++) {
            TreapNode* found = treap.find(&i);
            STD_INSIST(found != nullptr);
            STD_INSIST(static_cast<IntTreapNode*>(found)->value == i);
        }

        int key = 100;
        STD_INSIST(treap.find(&key) == nullptr);

        int key2 = -1;
        STD_INSIST(treap.find(&key2) == nullptr);
    }

    STD_TEST(NegativeValues) {
        IntTreap treap;
        IntTreapNode node1(-10);
        IntTreapNode node2(0);
        IntTreapNode node3(10);

        treap.insert(&node1);
        treap.insert(&node2);
        treap.insert(&node3);

        int key1 = -10;
        TreapNode* found1 = treap.find(&key1);
        STD_INSIST(found1 != nullptr);
        STD_INSIST(static_cast<IntTreapNode*>(found1)->value == -10);

        int key2 = 0;
        TreapNode* found2 = treap.find(&key2);
        STD_INSIST(found2 != nullptr);
        STD_INSIST(static_cast<IntTreapNode*>(found2)->value == 0);

        int key3 = 10;
        TreapNode* found3 = treap.find(&key3);
        STD_INSIST(found3 != nullptr);
        STD_INSIST(static_cast<IntTreapNode*>(found3)->value == 10);
    }

    STD_TEST(SingleElementFind) {
        IntTreap treap;
        IntTreapNode node(42);
        treap.insert(&node);

        int key1 = 41;
        STD_INSIST(treap.find(&key1) == nullptr);

        int key2 = 42;
        STD_INSIST(treap.find(&key2) != nullptr);

        int key3 = 43;
        STD_INSIST(treap.find(&key3) == nullptr);
    }

    STD_TEST(TwoElements) {
        IntTreap treap;
        IntTreapNode node1(10);
        IntTreapNode node2(20);

        treap.insert(&node1);
        treap.insert(&node2);

        int key1 = 10;
        TreapNode* found1 = treap.find(&key1);
        STD_INSIST(found1 != nullptr);
        STD_INSIST(static_cast<IntTreapNode*>(found1)->value == 10);

        int key2 = 20;
        TreapNode* found2 = treap.find(&key2);
        STD_INSIST(found2 != nullptr);
        STD_INSIST(static_cast<IntTreapNode*>(found2)->value == 20);

        int key3 = 15;
        STD_INSIST(treap.find(&key3) == nullptr);
    }

    STD_TEST(EraseSingleElement) {
        IntTreap treap;
        IntTreapNode node(42);
        treap.insert(&node);

        int key = 42;
        STD_INSIST(treap.find(&key) != nullptr);

        treap.erase(&key);
        STD_INSIST(treap.find(&key) == nullptr);
    }

    STD_TEST(EraseFromMultipleElements) {
        IntTreap treap;
        IntTreapNode node1(10);
        IntTreapNode node2(20);
        IntTreapNode node3(30);

        treap.insert(&node1);
        treap.insert(&node2);
        treap.insert(&node3);

        int key = 20;
        treap.erase(&key);

        STD_INSIST(treap.find(&key) == nullptr);

        int key1 = 10;
        STD_INSIST(treap.find(&key1) != nullptr);

        int key3 = 30;
        STD_INSIST(treap.find(&key3) != nullptr);
    }

    STD_TEST(EraseNonExistentKey) {
        IntTreap treap;
        IntTreapNode node1(10);
        IntTreapNode node2(20);

        treap.insert(&node1);
        treap.insert(&node2);

        int key = 15;
        treap.erase(&key);

        int key1 = 10;
        STD_INSIST(treap.find(&key1) != nullptr);

        int key2 = 20;
        STD_INSIST(treap.find(&key2) != nullptr);
    }

    STD_TEST(EraseAllElements) {
        IntTreap treap;
        IntTreapNode node1(10);
        IntTreapNode node2(20);
        IntTreapNode node3(30);

        treap.insert(&node1);
        treap.insert(&node2);
        treap.insert(&node3);

        int key1 = 10;
        treap.erase(&key1);
        STD_INSIST(treap.find(&key1) == nullptr);

        int key2 = 20;
        treap.erase(&key2);
        STD_INSIST(treap.find(&key2) == nullptr);

        int key3 = 30;
        treap.erase(&key3);
        STD_INSIST(treap.find(&key3) == nullptr);
    }

    STD_TEST(EraseAndReinsert) {
        IntTreap treap;
        IntTreapNode node1(10);
        IntTreapNode node2(20);

        treap.insert(&node1);

        int key1 = 10;
        treap.erase(&key1);
        STD_INSIST(treap.find(&key1) == nullptr);

        treap.insert(&node2);
        int key2 = 20;
        STD_INSIST(treap.find(&key2) != nullptr);
    }

    STD_TEST(EraseRootTreapNode) {
        IntTreap treap;
        IntTreapNode node1(20);
        IntTreapNode node2(10);
        IntTreapNode node3(30);

        treap.insert(&node1);
        treap.insert(&node2);
        treap.insert(&node3);

        int key1 = 20;
        treap.erase(&key1);
        STD_INSIST(treap.find(&key1) == nullptr);

        int key2 = 10;
        STD_INSIST(treap.find(&key2) != nullptr);

        int key3 = 30;
        STD_INSIST(treap.find(&key3) != nullptr);
    }

    STD_TEST(EraseLeafTreapNodes) {
        IntTreap treap;
        IntTreapNode node1(20);
        IntTreapNode node2(10);
        IntTreapNode node3(30);
        IntTreapNode node4(5);
        IntTreapNode node5(15);

        treap.insert(&node1);
        treap.insert(&node2);
        treap.insert(&node3);
        treap.insert(&node4);
        treap.insert(&node5);

        int key4 = 5;
        treap.erase(&key4);
        STD_INSIST(treap.find(&key4) == nullptr);

        int key5 = 15;
        treap.erase(&key5);
        STD_INSIST(treap.find(&key5) == nullptr);

        int key1 = 20;
        STD_INSIST(treap.find(&key1) != nullptr);
        int key2 = 10;
        STD_INSIST(treap.find(&key2) != nullptr);
        int key3 = 30;
        STD_INSIST(treap.find(&key3) != nullptr);
    }

    STD_TEST(EraseFromEmptyTreap) {
        IntTreap treap;
        int key = 42;
        treap.erase(&key);
        STD_INSIST(treap.find(&key) == nullptr);
    }

    STD_TEST(EraseSequentialElements) {
        IntTreap treap;
        IntTreapNode nodes[10] = {
            IntTreapNode(0), IntTreapNode(1), IntTreapNode(2), IntTreapNode(3), IntTreapNode(4),
            IntTreapNode(5), IntTreapNode(6), IntTreapNode(7), IntTreapNode(8), IntTreapNode(9)};

        for (int i = 0; i < 10; i++) {
            treap.insert(&nodes[i]);
        }

        for (int i = 0; i < 10; i += 2) {
            treap.erase(&i);
            STD_INSIST(treap.find(&i) == nullptr);
        }

        for (int i = 1; i < 10; i += 2) {
            STD_INSIST(treap.find(&i) != nullptr);
        }
    }
}
