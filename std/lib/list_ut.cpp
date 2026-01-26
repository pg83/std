#include "list.h"

#include <std/tst/ut.h>

using namespace Std;

STD_TEST_SUITE(IntrusiveList) {
    struct TestData: public IntrusiveNode {
        inline TestData(int v)
            : value(v)
        {
        }

        int value;
    };

    STD_TEST(emptyList) {
        IntrusiveList list;
        STD_INSIST(list.empty());
    }

    STD_TEST(pushBack) {
        IntrusiveList list;
        TestData d1(10);
        TestData d2(20);
        TestData d3(30);

        list.pushBack(&d1);
        STD_INSIST(!list.empty());
        STD_INSIST(list.mutFront() == &d1);
        STD_INSIST(list.mutBack() == &d1);

        list.pushBack(&d2);
        STD_INSIST(list.mutFront() == &d1);
        STD_INSIST(list.mutBack() == &d2);

        list.pushBack(&d3);
        STD_INSIST(list.mutFront() == &d1);
        STD_INSIST(list.mutBack() == &d3);

        IntrusiveNode* head = list.mutEnd();
        IntrusiveNode* current = head->next;
        int values[] = {10, 20, 30};
        int idx = 0;
        while (current != head) {
            TestData* data = static_cast<TestData*>(current);
            STD_INSIST(data->value == values[idx++]);
            current = current->next;
        }
        STD_INSIST(idx == 3);
    }

    STD_TEST(pushFront) {
        IntrusiveList list;
        TestData d1(10);
        TestData d2(20);
        TestData d3(30);

        list.pushFront(&d1);
        STD_INSIST(!list.empty());
        STD_INSIST(list.mutFront() == &d1);
        STD_INSIST(list.mutBack() == &d1);

        list.pushFront(&d2);
        STD_INSIST(list.mutFront() == &d2);
        STD_INSIST(list.mutBack() == &d1);

        list.pushFront(&d3);
        STD_INSIST(list.mutFront() == &d3);
        STD_INSIST(list.mutBack() == &d1);

        IntrusiveNode* head = list.mutEnd();
        IntrusiveNode* current = head->next;
        int values[] = {30, 20, 10};
        int idx = 0;
        while (current != head) {
            TestData* data = static_cast<TestData*>(current);
            STD_INSIST(data->value == values[idx++]);
            current = current->next;
        }
        STD_INSIST(idx == 3);
    }

    STD_TEST(popFront) {
        IntrusiveList list;
        TestData d1(10);
        TestData d2(20);
        TestData d3(30);

        list.pushBack(&d1);
        list.pushBack(&d2);
        list.pushBack(&d3);

        IntrusiveNode* node = list.popFront();
        STD_INSIST(node == &d1);
        TestData* data = static_cast<TestData*>(node);
        STD_INSIST(data->value == 10);
        STD_INSIST(list.mutFront() == &d2);

        node = list.popFront();
        STD_INSIST(node == &d2);
        data = static_cast<TestData*>(node);
        STD_INSIST(data->value == 20);
        STD_INSIST(list.mutFront() == &d3);

        node = list.popFront();
        STD_INSIST(node == &d3);
        data = static_cast<TestData*>(node);
        STD_INSIST(data->value == 30);
        STD_INSIST(list.empty());
    }

    STD_TEST(popBack) {
        IntrusiveList list;
        TestData d1(10);
        TestData d2(20);
        TestData d3(30);

        list.pushBack(&d1);
        list.pushBack(&d2);
        list.pushBack(&d3);

        IntrusiveNode* node = list.popBack();
        STD_INSIST(node == &d3);
        TestData* data = static_cast<TestData*>(node);
        STD_INSIST(data->value == 30);
        STD_INSIST(list.mutBack() == &d2);

        node = list.popBack();
        STD_INSIST(node == &d2);
        data = static_cast<TestData*>(node);
        STD_INSIST(data->value == 20);
        STD_INSIST(list.mutBack() == &d1);

        node = list.popBack();
        STD_INSIST(node == &d1);
        data = static_cast<TestData*>(node);
        STD_INSIST(data->value == 10);
        STD_INSIST(list.empty());
    }

    STD_TEST(removeMiddle) {
        IntrusiveList list;
        TestData d1(10);
        TestData d2(20);
        TestData d3(30);
        TestData d4(40);

        list.pushBack(&d1);
        list.pushBack(&d2);
        list.pushBack(&d3);
        list.pushBack(&d4);

        d2.remove();

        IntrusiveNode* head = list.mutEnd();
        IntrusiveNode* current = head->next;
        int values[] = {10, 30, 40};
        int idx = 0;
        while (current != head) {
            TestData* data = static_cast<TestData*>(current);
            STD_INSIST(data->value == values[idx++]);
            current = current->next;
        }
        STD_INSIST(idx == 3);

        d3.remove();

        current = head->next;
        int values2[] = {10, 40};
        idx = 0;
        while (current != head) {
            TestData* data = static_cast<TestData*>(current);
            STD_INSIST(data->value == values2[idx++]);
            current = current->next;
        }
        STD_INSIST(idx == 2);
    }

    STD_TEST(insertAfterFunction) {
        IntrusiveList list;
        TestData d1(10);
        TestData d2(20);
        TestData d3(30);
        TestData d4(15);

        list.pushBack(&d1);
        list.pushBack(&d2);
        list.pushBack(&d3);

        IntrusiveList::insertAfter(&d1, &d4);

        IntrusiveNode* head = list.mutEnd();
        IntrusiveNode* current = head->next;
        int values[] = {10, 15, 20, 30};
        int idx = 0;
        while (current != head) {
            TestData* data = static_cast<TestData*>(current);
            STD_INSIST(data->value == values[idx++]);
            current = current->next;
        }
        STD_INSIST(idx == 4);
    }

    STD_TEST(insertBeforeFunction) {
        IntrusiveList list;
        TestData d1(10);
        TestData d2(20);
        TestData d3(30);
        TestData d4(25);

        list.pushBack(&d1);
        list.pushBack(&d2);
        list.pushBack(&d3);

        IntrusiveList::insertBefore(&d3, &d4);

        IntrusiveNode* head = list.mutEnd();
        IntrusiveNode* current = head->next;
        int values[] = {10, 20, 25, 30};
        int idx = 0;
        while (current != head) {
            TestData* data = static_cast<TestData*>(current);
            STD_INSIST(data->value == values[idx++]);
            current = current->next;
        }
        STD_INSIST(idx == 4);
    }

    STD_TEST(clear) {
        IntrusiveList list;
        TestData d1(10);
        TestData d2(20);
        TestData d3(30);

        list.pushBack(&d1);
        list.pushBack(&d2);
        list.pushBack(&d3);

        STD_INSIST(!list.empty());

        list.clear();

        STD_INSIST(list.empty());
    }

    STD_TEST(mixedOperations) {
        IntrusiveList list;
        TestData d1(1);
        TestData d2(2);
        TestData d3(3);
        TestData d4(4);
        TestData d5(5);

        list.pushBack(&d1);
        list.pushFront(&d2);
        list.pushBack(&d3);
        list.pushFront(&d4);
        list.pushBack(&d5);

        IntrusiveNode* head = list.mutEnd();
        IntrusiveNode* current = head->next;
        int values[] = {4, 2, 1, 3, 5};
        int idx = 0;
        while (current != head) {
            TestData* data = static_cast<TestData*>(current);
            STD_INSIST(data->value == values[idx++]);
            current = current->next;
        }
        STD_INSIST(idx == 5);

        d1.remove();
        d3.remove();

        current = head->next;
        int values2[] = {4, 2, 5};
        idx = 0;
        while (current != head) {
            TestData* data = static_cast<TestData*>(current);
            STD_INSIST(data->value == values2[idx++]);
            current = current->next;
        }
        STD_INSIST(idx == 3);
    }

    STD_TEST(backwardTraversal) {
        IntrusiveList list;
        TestData d1(10);
        TestData d2(20);
        TestData d3(30);

        list.pushBack(&d1);
        list.pushBack(&d2);
        list.pushBack(&d3);

        IntrusiveNode* head = list.mutEnd();
        IntrusiveNode* current = head->prev;
        int values[] = {30, 20, 10};
        int idx = 0;
        while (current != head) {
            TestData* data = static_cast<TestData*>(current);
            STD_INSIST(data->value == values[idx++]);
            current = current->prev;
        }
        STD_INSIST(idx == 3);
    }

    STD_TEST(singleElement) {
        IntrusiveList list;
        TestData d1(42);

        list.pushBack(&d1);
        STD_INSIST(!list.empty());
        STD_INSIST(list.mutFront() == &d1);
        STD_INSIST(list.mutBack() == &d1);

        IntrusiveNode* node = list.popFront();
        STD_INSIST(node == &d1);
        STD_INSIST(list.empty());
    }

    STD_TEST(reinsertAfterRemove) {
        IntrusiveList list;
        TestData d1(10);
        TestData d2(20);

        list.pushBack(&d1);
        list.pushBack(&d2);

        d1.remove();
        STD_INSIST(list.mutFront() == &d2);

        list.pushFront(&d1);
        STD_INSIST(list.mutFront() == &d1);
        STD_INSIST(list.mutBack() == &d2);

        IntrusiveNode* head = list.mutEnd();
        IntrusiveNode* current = head->next;
        int values[] = {10, 20};
        int idx = 0;
        while (current != head) {
            TestData* data = static_cast<TestData*>(current);
            STD_INSIST(data->value == values[idx++]);
            current = current->next;
        }
        STD_INSIST(idx == 2);
    }
}
