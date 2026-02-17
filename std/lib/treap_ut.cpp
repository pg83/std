#include "treap.h"

#include <std/tst/ut.h>

using namespace Std;

STD_TEST_SUITE(Treap) {
    struct IntNode: public Node {
        int value;

        IntNode(int v)
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
        IntNode node(10);
        treap.insert(&node);

        int key = 10;
        Node* found = treap.find(&key);
        STD_INSIST(found != nullptr);
        STD_INSIST(static_cast<IntNode*>(found)->value == 10);
    }

    STD_TEST(InsertMultipleElements) {
        IntTreap treap;
        IntNode node1(5);
        IntNode node2(10);
        IntNode node3(15);

        treap.insert(&node1);
        treap.insert(&node2);
        treap.insert(&node3);

        int key1 = 5;
        Node* found1 = treap.find(&key1);
        STD_INSIST(found1 != nullptr);
        STD_INSIST(static_cast<IntNode*>(found1)->value == 5);

        int key2 = 10;
        Node* found2 = treap.find(&key2);
        STD_INSIST(found2 != nullptr);
        STD_INSIST(static_cast<IntNode*>(found2)->value == 10);

        int key3 = 15;
        Node* found3 = treap.find(&key3);
        STD_INSIST(found3 != nullptr);
        STD_INSIST(static_cast<IntNode*>(found3)->value == 15);
    }

    STD_TEST(FindNonExistentKey) {
        IntTreap treap;
        IntNode node1(10);
        IntNode node2(20);
        IntNode node3(30);

        treap.insert(&node1);
        treap.insert(&node2);
        treap.insert(&node3);

        int key = 15;
        Node* found = treap.find(&key);
        STD_INSIST(found == nullptr);
    }

    STD_TEST(InsertAscendingOrder) {
        IntTreap treap;
        IntNode node1(1);
        IntNode node2(2);
        IntNode node3(3);
        IntNode node4(4);
        IntNode node5(5);

        treap.insert(&node1);
        treap.insert(&node2);
        treap.insert(&node3);
        treap.insert(&node4);
        treap.insert(&node5);

        for (int i = 1; i <= 5; i++) {
            Node* found = treap.find(&i);
            STD_INSIST(found != nullptr);
            STD_INSIST(static_cast<IntNode*>(found)->value == i);
        }
    }

    STD_TEST(InsertDescendingOrder) {
        IntTreap treap;
        IntNode node1(5);
        IntNode node2(4);
        IntNode node3(3);
        IntNode node4(2);
        IntNode node5(1);

        treap.insert(&node1);
        treap.insert(&node2);
        treap.insert(&node3);
        treap.insert(&node4);
        treap.insert(&node5);

        for (int i = 1; i <= 5; i++) {
            Node* found = treap.find(&i);
            STD_INSIST(found != nullptr);
            STD_INSIST(static_cast<IntNode*>(found)->value == i);
        }
    }

    STD_TEST(InsertRandomOrder) {
        IntTreap treap;
        IntNode node1(15);
        IntNode node2(5);
        IntNode node3(25);
        IntNode node4(3);
        IntNode node5(20);
        IntNode node6(30);
        IntNode node7(10);

        treap.insert(&node1);
        treap.insert(&node2);
        treap.insert(&node3);
        treap.insert(&node4);
        treap.insert(&node5);
        treap.insert(&node6);
        treap.insert(&node7);

        int keys[] = {3, 5, 10, 15, 20, 25, 30};
        for (int key : keys) {
            Node* found = treap.find(&key);
            STD_INSIST(found != nullptr);
            STD_INSIST(static_cast<IntNode*>(found)->value == key);
        }
    }

    STD_TEST(FindBoundaryKeys) {
        IntTreap treap;
        IntNode node1(10);
        IntNode node2(20);
        IntNode node3(30);

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
        IntNode node1(10);
        IntNode node2(10);
        IntNode node3(10);

        treap.insert(&node1);
        treap.insert(&node2);
        treap.insert(&node3);

        int key = 10;
        Node* found = treap.find(&key);
        STD_INSIST(found != nullptr);
        STD_INSIST(static_cast<IntNode*>(found)->value == 10);
    }

    STD_TEST(InsertManyElements) {
        IntTreap treap;
        IntNode nodes[100] = {
            IntNode(0), IntNode(1), IntNode(2), IntNode(3), IntNode(4),
            IntNode(5), IntNode(6), IntNode(7), IntNode(8), IntNode(9),
            IntNode(10), IntNode(11), IntNode(12), IntNode(13), IntNode(14),
            IntNode(15), IntNode(16), IntNode(17), IntNode(18), IntNode(19),
            IntNode(20), IntNode(21), IntNode(22), IntNode(23), IntNode(24),
            IntNode(25), IntNode(26), IntNode(27), IntNode(28), IntNode(29),
            IntNode(30), IntNode(31), IntNode(32), IntNode(33), IntNode(34),
            IntNode(35), IntNode(36), IntNode(37), IntNode(38), IntNode(39),
            IntNode(40), IntNode(41), IntNode(42), IntNode(43), IntNode(44),
            IntNode(45), IntNode(46), IntNode(47), IntNode(48), IntNode(49),
            IntNode(50), IntNode(51), IntNode(52), IntNode(53), IntNode(54),
            IntNode(55), IntNode(56), IntNode(57), IntNode(58), IntNode(59),
            IntNode(60), IntNode(61), IntNode(62), IntNode(63), IntNode(64),
            IntNode(65), IntNode(66), IntNode(67), IntNode(68), IntNode(69),
            IntNode(70), IntNode(71), IntNode(72), IntNode(73), IntNode(74),
            IntNode(75), IntNode(76), IntNode(77), IntNode(78), IntNode(79),
            IntNode(80), IntNode(81), IntNode(82), IntNode(83), IntNode(84),
            IntNode(85), IntNode(86), IntNode(87), IntNode(88), IntNode(89),
            IntNode(90), IntNode(91), IntNode(92), IntNode(93), IntNode(94),
            IntNode(95), IntNode(96), IntNode(97), IntNode(98), IntNode(99)};

        for (int i = 0; i < 100; i++) {
            treap.insert(&nodes[i]);
        }

        for (int i = 0; i < 100; i++) {
            Node* found = treap.find(&i);
            STD_INSIST(found != nullptr);
            STD_INSIST(static_cast<IntNode*>(found)->value == i);
        }

        int key = 100;
        STD_INSIST(treap.find(&key) == nullptr);

        int key2 = -1;
        STD_INSIST(treap.find(&key2) == nullptr);
    }

    STD_TEST(NegativeValues) {
        IntTreap treap;
        IntNode node1(-10);
        IntNode node2(0);
        IntNode node3(10);

        treap.insert(&node1);
        treap.insert(&node2);
        treap.insert(&node3);

        int key1 = -10;
        Node* found1 = treap.find(&key1);
        STD_INSIST(found1 != nullptr);
        STD_INSIST(static_cast<IntNode*>(found1)->value == -10);

        int key2 = 0;
        Node* found2 = treap.find(&key2);
        STD_INSIST(found2 != nullptr);
        STD_INSIST(static_cast<IntNode*>(found2)->value == 0);

        int key3 = 10;
        Node* found3 = treap.find(&key3);
        STD_INSIST(found3 != nullptr);
        STD_INSIST(static_cast<IntNode*>(found3)->value == 10);
    }

    STD_TEST(SingleElementFind) {
        IntTreap treap;
        IntNode node(42);
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
        IntNode node1(10);
        IntNode node2(20);

        treap.insert(&node1);
        treap.insert(&node2);

        int key1 = 10;
        Node* found1 = treap.find(&key1);
        STD_INSIST(found1 != nullptr);
        STD_INSIST(static_cast<IntNode*>(found1)->value == 10);

        int key2 = 20;
        Node* found2 = treap.find(&key2);
        STD_INSIST(found2 != nullptr);
        STD_INSIST(static_cast<IntNode*>(found2)->value == 20);

        int key3 = 15;
        STD_INSIST(treap.find(&key3) == nullptr);
    }

    STD_TEST(EraseSingleElement) {
        IntTreap treap;
        IntNode node(42);
        treap.insert(&node);

        int key = 42;
        STD_INSIST(treap.find(&key) != nullptr);

        treap.erase(&key);
        STD_INSIST(treap.find(&key) == nullptr);
    }

    STD_TEST(EraseFromMultipleElements) {
        IntTreap treap;
        IntNode node1(10);
        IntNode node2(20);
        IntNode node3(30);

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
        IntNode node1(10);
        IntNode node2(20);

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
        IntNode node1(10);
        IntNode node2(20);
        IntNode node3(30);

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
        IntNode node1(10);
        IntNode node2(20);

        treap.insert(&node1);

        int key1 = 10;
        treap.erase(&key1);
        STD_INSIST(treap.find(&key1) == nullptr);

        treap.insert(&node2);
        int key2 = 20;
        STD_INSIST(treap.find(&key2) != nullptr);
    }

    STD_TEST(EraseRootNode) {
        IntTreap treap;
        IntNode node1(20);
        IntNode node2(10);
        IntNode node3(30);

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

    STD_TEST(EraseLeafNodes) {
        IntTreap treap;
        IntNode node1(20);
        IntNode node2(10);
        IntNode node3(30);
        IntNode node4(5);
        IntNode node5(15);

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
        IntNode nodes[10] = {
            IntNode(0), IntNode(1), IntNode(2), IntNode(3), IntNode(4),
            IntNode(5), IntNode(6), IntNode(7), IntNode(8), IntNode(9)};

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
