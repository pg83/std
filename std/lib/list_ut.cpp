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

    STD_TEST(xchgBothEmpty) {
        IntrusiveList list1;
        IntrusiveList list2;

        STD_INSIST(list1.empty());
        STD_INSIST(list2.empty());

        list1.xchg(list2);

        STD_INSIST(list1.empty());
        STD_INSIST(list2.empty());
    }

    STD_TEST(xchgFirstEmptySecondNonEmpty) {
        IntrusiveList list1;
        IntrusiveList list2;
        TestData d1(10);
        TestData d2(20);
        TestData d3(30);

        list2.pushBack(&d1);
        list2.pushBack(&d2);
        list2.pushBack(&d3);

        STD_INSIST(list1.empty());
        STD_INSIST(!list2.empty());

        list1.xchg(list2);

        STD_INSIST(!list1.empty());
        STD_INSIST(list2.empty());

        IntrusiveNode* head = list1.mutEnd();
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

    STD_TEST(xchgFirstNonEmptySecondEmpty) {
        IntrusiveList list1;
        IntrusiveList list2;
        TestData d1(10);
        TestData d2(20);
        TestData d3(30);

        list1.pushBack(&d1);
        list1.pushBack(&d2);
        list1.pushBack(&d3);

        STD_INSIST(!list1.empty());
        STD_INSIST(list2.empty());

        list1.xchg(list2);

        STD_INSIST(list1.empty());
        STD_INSIST(!list2.empty());

        IntrusiveNode* head = list2.mutEnd();
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

    STD_TEST(xchgBothNonEmpty) {
        IntrusiveList list1;
        IntrusiveList list2;
        TestData d1(10);
        TestData d2(20);
        TestData d3(30);
        TestData d4(40);
        TestData d5(50);

        list1.pushBack(&d1);
        list1.pushBack(&d2);
        list1.pushBack(&d3);

        list2.pushBack(&d4);
        list2.pushBack(&d5);

        list1.xchg(list2);

        IntrusiveNode* head1 = list1.mutEnd();
        IntrusiveNode* current1 = head1->next;
        int values1[] = {40, 50};
        int idx1 = 0;
        while (current1 != head1) {
            TestData* data = static_cast<TestData*>(current1);
            STD_INSIST(data->value == values1[idx1++]);
            current1 = current1->next;
        }
        STD_INSIST(idx1 == 2);

        IntrusiveNode* head2 = list2.mutEnd();
        IntrusiveNode* current2 = head2->next;
        int values2[] = {10, 20, 30};
        int idx2 = 0;
        while (current2 != head2) {
            TestData* data = static_cast<TestData*>(current2);
            STD_INSIST(data->value == values2[idx2++]);
            current2 = current2->next;
        }
        STD_INSIST(idx2 == 3);
    }

    STD_TEST(xchgSingleElementLists) {
        IntrusiveList list1;
        IntrusiveList list2;
        TestData d1(100);
        TestData d2(200);

        list1.pushBack(&d1);
        list2.pushBack(&d2);

        list1.xchg(list2);

        STD_INSIST(list1.mutFront() == &d2);
        STD_INSIST(list1.mutBack() == &d2);
        STD_INSIST(list2.mutFront() == &d1);
        STD_INSIST(list2.mutBack() == &d1);

        TestData* data1 = static_cast<TestData*>(list1.mutFront());
        STD_INSIST(data1->value == 200);

        TestData* data2 = static_cast<TestData*>(list2.mutFront());
        STD_INSIST(data2->value == 100);
    }

    STD_TEST(xchgMultipleTimes) {
        IntrusiveList list1;
        IntrusiveList list2;
        TestData d1(1);
        TestData d2(2);
        TestData d3(3);

        list1.pushBack(&d1);
        list2.pushBack(&d2);
        list2.pushBack(&d3);

        list1.xchg(list2);

        STD_INSIST(list1.mutFront() == &d2);
        STD_INSIST(list2.mutFront() == &d1);

        list1.xchg(list2);

        STD_INSIST(list1.mutFront() == &d1);
        STD_INSIST(list2.mutFront() == &d2);

        list1.xchg(list2);

        STD_INSIST(list1.mutFront() == &d2);
        STD_INSIST(list2.mutFront() == &d1);
    }

    STD_TEST(xchgThenModify) {
        IntrusiveList list1;
        IntrusiveList list2;
        TestData d1(1);
        TestData d2(2);
        TestData d3(3);
        TestData d4(4);

        list1.pushBack(&d1);
        list1.pushBack(&d2);
        list2.pushBack(&d3);

        list1.xchg(list2);

        list1.pushBack(&d4);

        IntrusiveNode* head1 = list1.mutEnd();
        IntrusiveNode* current1 = head1->next;
        int values1[] = {3, 4};
        int idx1 = 0;
        while (current1 != head1) {
            TestData* data = static_cast<TestData*>(current1);
            STD_INSIST(data->value == values1[idx1++]);
            current1 = current1->next;
        }
        STD_INSIST(idx1 == 2);

        IntrusiveNode* head2 = list2.mutEnd();
        IntrusiveNode* current2 = head2->next;
        int values2[] = {1, 2};
        int idx2 = 0;
        while (current2 != head2) {
            TestData* data = static_cast<TestData*>(current2);
            STD_INSIST(data->value == values2[idx2++]);
            current2 = current2->next;
        }
        STD_INSIST(idx2 == 2);
    }

    STD_TEST(xchgBackwardTraversal) {
        IntrusiveList list1;
        IntrusiveList list2;
        TestData d1(10);
        TestData d2(20);
        TestData d3(30);
        TestData d4(40);
        TestData d5(50);

        list1.pushBack(&d1);
        list1.pushBack(&d2);

        list2.pushBack(&d3);
        list2.pushBack(&d4);
        list2.pushBack(&d5);

        list1.xchg(list2);

        IntrusiveNode* head1 = list1.mutEnd();
        IntrusiveNode* current1 = head1->prev;
        int values1[] = {50, 40, 30};
        int idx1 = 0;
        while (current1 != head1) {
            TestData* data = static_cast<TestData*>(current1);
            STD_INSIST(data->value == values1[idx1++]);
            current1 = current1->prev;
        }
        STD_INSIST(idx1 == 3);

        IntrusiveNode* head2 = list2.mutEnd();
        IntrusiveNode* current2 = head2->prev;
        int values2[] = {20, 10};
        int idx2 = 0;
        while (current2 != head2) {
            TestData* data = static_cast<TestData*>(current2);
            STD_INSIST(data->value == values2[idx2++]);
            current2 = current2->prev;
        }
        STD_INSIST(idx2 == 2);
    }

    STD_TEST(xchgWithEmptyListMethod) {
        IntrusiveList list1;
        IntrusiveList list2;
        TestData d1(10);
        TestData d2(20);

        list1.pushBack(&d1);
        list1.pushBack(&d2);

        STD_INSIST(!list1.empty());
        STD_INSIST(list2.empty());

        list1.xchgWithEmptyList(list2);

        STD_INSIST(list1.empty());
        STD_INSIST(!list2.empty());

        IntrusiveNode* head = list2.mutEnd();
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

    STD_TEST(xchgFrontBackPointers) {
        IntrusiveList list1;
        IntrusiveList list2;
        TestData d1(1);
        TestData d2(2);
        TestData d3(3);
        TestData d4(4);

        list1.pushBack(&d1);
        list1.pushBack(&d2);
        list2.pushBack(&d3);
        list2.pushBack(&d4);

        list1.xchg(list2);

        STD_INSIST(list1.mutFront() == &d3);
        STD_INSIST(list1.mutBack() == &d4);
        STD_INSIST(list2.mutFront() == &d1);
        STD_INSIST(list2.mutBack() == &d2);
    }

    STD_TEST(xchgLength) {
        IntrusiveList list1;
        IntrusiveList list2;
        TestData d1(1);
        TestData d2(2);
        TestData d3(3);

        list1.pushBack(&d1);
        list2.pushBack(&d2);
        list2.pushBack(&d3);

        STD_INSIST(list1.length() == 1);
        STD_INSIST(list2.length() == 2);

        list1.xchg(list2);

        STD_INSIST(list1.length() == 2);
        STD_INSIST(list2.length() == 1);
    }

    static bool ascendingComparator(const IntrusiveNode* a, const IntrusiveNode* b) {
        const TestData* ta = static_cast<const TestData*>(a);
        const TestData* tb = static_cast<const TestData*>(b);
        return ta->value < tb->value;
    }

    static bool descendingComparator(const IntrusiveNode* a, const IntrusiveNode* b) {
        const TestData* ta = static_cast<const TestData*>(a);
        const TestData* tb = static_cast<const TestData*>(b);
        return ta->value > tb->value;
    }

    STD_TEST(sortEmptyList) {
        IntrusiveList list;
        list.sort(ascendingComparator);
        STD_INSIST(list.empty());
    }

    STD_TEST(sortSingleElement) {
        IntrusiveList list;
        TestData d1(42);
        list.pushBack(&d1);

        list.sort(ascendingComparator);

        STD_INSIST(list.mutFront() == &d1);
        STD_INSIST(list.mutBack() == &d1);
        TestData* data = static_cast<TestData*>(list.mutFront());
        STD_INSIST(data->value == 42);
    }

    STD_TEST(sortTwoElementsAlreadySorted) {
        IntrusiveList list;
        TestData d1(10);
        TestData d2(20);

        list.pushBack(&d1);
        list.pushBack(&d2);

        list.sort(ascendingComparator);

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

    STD_TEST(sortTwoElementsReversed) {
        IntrusiveList list;
        TestData d1(20);
        TestData d2(10);

        list.pushBack(&d1);
        list.pushBack(&d2);

        list.sort(ascendingComparator);

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

    STD_TEST(sortAscendingMultipleElements) {
        IntrusiveList list;
        TestData d1(50);
        TestData d2(20);
        TestData d3(80);
        TestData d4(10);
        TestData d5(30);

        list.pushBack(&d1);
        list.pushBack(&d2);
        list.pushBack(&d3);
        list.pushBack(&d4);
        list.pushBack(&d5);

        list.sort(ascendingComparator);

        IntrusiveNode* head = list.mutEnd();
        IntrusiveNode* current = head->next;
        int values[] = {10, 20, 30, 50, 80};
        int idx = 0;
        while (current != head) {
            TestData* data = static_cast<TestData*>(current);
            STD_INSIST(data->value == values[idx++]);
            current = current->next;
        }
        STD_INSIST(idx == 5);
    }

    STD_TEST(sortDescending) {
        IntrusiveList list;
        TestData d1(10);
        TestData d2(50);
        TestData d3(30);
        TestData d4(20);
        TestData d5(40);

        list.pushBack(&d1);
        list.pushBack(&d2);
        list.pushBack(&d3);
        list.pushBack(&d4);
        list.pushBack(&d5);

        list.sort(descendingComparator);

        IntrusiveNode* head = list.mutEnd();
        IntrusiveNode* current = head->next;
        int values[] = {50, 40, 30, 20, 10};
        int idx = 0;
        while (current != head) {
            TestData* data = static_cast<TestData*>(current);
            STD_INSIST(data->value == values[idx++]);
            current = current->next;
        }
        STD_INSIST(idx == 5);
    }

    STD_TEST(sortAlreadySorted) {
        IntrusiveList list;
        TestData d1(10);
        TestData d2(20);
        TestData d3(30);
        TestData d4(40);
        TestData d5(50);

        list.pushBack(&d1);
        list.pushBack(&d2);
        list.pushBack(&d3);
        list.pushBack(&d4);
        list.pushBack(&d5);

        list.sort(ascendingComparator);

        IntrusiveNode* head = list.mutEnd();
        IntrusiveNode* current = head->next;
        int values[] = {10, 20, 30, 40, 50};
        int idx = 0;
        while (current != head) {
            TestData* data = static_cast<TestData*>(current);
            STD_INSIST(data->value == values[idx++]);
            current = current->next;
        }
        STD_INSIST(idx == 5);
    }

    STD_TEST(sortReversed) {
        IntrusiveList list;
        TestData d1(50);
        TestData d2(40);
        TestData d3(30);
        TestData d4(20);
        TestData d5(10);

        list.pushBack(&d1);
        list.pushBack(&d2);
        list.pushBack(&d3);
        list.pushBack(&d4);
        list.pushBack(&d5);

        list.sort(ascendingComparator);

        IntrusiveNode* head = list.mutEnd();
        IntrusiveNode* current = head->next;
        int values[] = {10, 20, 30, 40, 50};
        int idx = 0;
        while (current != head) {
            TestData* data = static_cast<TestData*>(current);
            STD_INSIST(data->value == values[idx++]);
            current = current->next;
        }
        STD_INSIST(idx == 5);
    }

    STD_TEST(sortDuplicateValues) {
        IntrusiveList list;
        TestData d1(30);
        TestData d2(10);
        TestData d3(20);
        TestData d4(20);
        TestData d5(10);
        TestData d6(30);

        list.pushBack(&d1);
        list.pushBack(&d2);
        list.pushBack(&d3);
        list.pushBack(&d4);
        list.pushBack(&d5);
        list.pushBack(&d6);

        list.sort(ascendingComparator);

        IntrusiveNode* head = list.mutEnd();
        IntrusiveNode* current = head->next;
        int values[] = {10, 10, 20, 20, 30, 30};
        int idx = 0;
        while (current != head) {
            TestData* data = static_cast<TestData*>(current);
            STD_INSIST(data->value == values[idx++]);
            current = current->next;
        }
        STD_INSIST(idx == 6);
    }

    STD_TEST(sortAllSameValues) {
        IntrusiveList list;
        TestData d1(42);
        TestData d2(42);
        TestData d3(42);
        TestData d4(42);

        list.pushBack(&d1);
        list.pushBack(&d2);
        list.pushBack(&d3);
        list.pushBack(&d4);

        list.sort(ascendingComparator);

        IntrusiveNode* head = list.mutEnd();
        IntrusiveNode* current = head->next;
        int count = 0;
        while (current != head) {
            TestData* data = static_cast<TestData*>(current);
            STD_INSIST(data->value == 42);
            current = current->next;
            count++;
        }
        STD_INSIST(count == 4);
    }

    STD_TEST(sortNegativeValues) {
        IntrusiveList list;
        TestData d1(-10);
        TestData d2(20);
        TestData d3(-5);
        TestData d4(0);
        TestData d5(-15);

        list.pushBack(&d1);
        list.pushBack(&d2);
        list.pushBack(&d3);
        list.pushBack(&d4);
        list.pushBack(&d5);

        list.sort(ascendingComparator);

        IntrusiveNode* head = list.mutEnd();
        IntrusiveNode* current = head->next;
        int values[] = {-15, -10, -5, 0, 20};
        int idx = 0;
        while (current != head) {
            TestData* data = static_cast<TestData*>(current);
            STD_INSIST(data->value == values[idx++]);
            current = current->next;
        }
        STD_INSIST(idx == 5);
    }

    STD_TEST(sortLargeList) {
        IntrusiveList list;
        TestData d1(100);
        TestData d2(25);
        TestData d3(75);
        TestData d4(10);
        TestData d5(90);
        TestData d6(5);
        TestData d7(50);
        TestData d8(30);
        TestData d9(60);
        TestData d10(80);

        list.pushBack(&d1);
        list.pushBack(&d2);
        list.pushBack(&d3);
        list.pushBack(&d4);
        list.pushBack(&d5);
        list.pushBack(&d6);
        list.pushBack(&d7);
        list.pushBack(&d8);
        list.pushBack(&d9);
        list.pushBack(&d10);

        list.sort(ascendingComparator);

        IntrusiveNode* head = list.mutEnd();
        IntrusiveNode* current = head->next;
        int values[] = {5, 10, 25, 30, 50, 60, 75, 80, 90, 100};
        int idx = 0;
        while (current != head) {
            TestData* data = static_cast<TestData*>(current);
            STD_INSIST(data->value == values[idx++]);
            current = current->next;
        }
        STD_INSIST(idx == 10);
    }

    STD_TEST(sortBackwardTraversalAfterSort) {
        IntrusiveList list;
        TestData d1(30);
        TestData d2(10);
        TestData d3(50);
        TestData d4(20);
        TestData d5(40);

        list.pushBack(&d1);
        list.pushBack(&d2);
        list.pushBack(&d3);
        list.pushBack(&d4);
        list.pushBack(&d5);

        list.sort(ascendingComparator);

        IntrusiveNode* head = list.mutEnd();
        IntrusiveNode* current = head->prev;
        int values[] = {50, 40, 30, 20, 10};
        int idx = 0;
        while (current != head) {
            TestData* data = static_cast<TestData*>(current);
            STD_INSIST(data->value == values[idx++]);
            current = current->prev;
        }
        STD_INSIST(idx == 5);
    }

    STD_TEST(sortFrontAndBackPointers) {
        IntrusiveList list;
        TestData d1(40);
        TestData d2(10);
        TestData d3(30);
        TestData d4(20);

        list.pushBack(&d1);
        list.pushBack(&d2);
        list.pushBack(&d3);
        list.pushBack(&d4);

        list.sort(ascendingComparator);

        TestData* front = static_cast<TestData*>(list.mutFront());
        TestData* back = static_cast<TestData*>(list.mutBack());

        STD_INSIST(front->value == 10);
        STD_INSIST(back->value == 40);
    }

    STD_TEST(sortPreservesLength) {
        IntrusiveList list;
        TestData d1(50);
        TestData d2(20);
        TestData d3(80);
        TestData d4(10);
        TestData d5(30);

        list.pushBack(&d1);
        list.pushBack(&d2);
        list.pushBack(&d3);
        list.pushBack(&d4);
        list.pushBack(&d5);

        unsigned lengthBefore = list.length();
        list.sort(ascendingComparator);
        unsigned lengthAfter = list.length();

        STD_INSIST(lengthBefore == 5);
        STD_INSIST(lengthAfter == 5);
    }

    STD_TEST(sortThenModify) {
        IntrusiveList list;
        TestData d1(30);
        TestData d2(10);
        TestData d3(20);
        TestData d4(5);

        list.pushBack(&d1);
        list.pushBack(&d2);
        list.pushBack(&d3);

        list.sort(ascendingComparator);

        list.pushBack(&d4);

        IntrusiveNode* head = list.mutEnd();
        IntrusiveNode* current = head->next;
        int values[] = {10, 20, 30, 5};
        int idx = 0;
        while (current != head) {
            TestData* data = static_cast<TestData*>(current);
            STD_INSIST(data->value == values[idx++]);
            current = current->next;
        }
        STD_INSIST(idx == 4);
    }

    STD_TEST(sortMultipleTimes) {
        IntrusiveList list;
        TestData d1(30);
        TestData d2(10);
        TestData d3(20);

        list.pushBack(&d1);
        list.pushBack(&d2);
        list.pushBack(&d3);

        list.sort(ascendingComparator);

        IntrusiveNode* head = list.mutEnd();
        IntrusiveNode* current = head->next;
        int values1[] = {10, 20, 30};
        int idx1 = 0;
        while (current != head) {
            TestData* data = static_cast<TestData*>(current);
            STD_INSIST(data->value == values1[idx1++]);
            current = current->next;
        }
        STD_INSIST(idx1 == 3);

        list.sort(descendingComparator);

        head = list.mutEnd();
        current = head->next;
        int values2[] = {30, 20, 10};
        int idx2 = 0;
        while (current != head) {
            TestData* data = static_cast<TestData*>(current);
            STD_INSIST(data->value == values2[idx2++]);
            current = current->next;
        }
        STD_INSIST(idx2 == 3);
    }

    STD_TEST(sortThreeElements) {
        IntrusiveList list;
        TestData d1(30);
        TestData d2(10);
        TestData d3(20);

        list.pushBack(&d1);
        list.pushBack(&d2);
        list.pushBack(&d3);

        list.sort(ascendingComparator);

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

    STD_TEST(sortOddNumberOfElements) {
        IntrusiveList list;
        TestData d1(70);
        TestData d2(30);
        TestData d3(90);
        TestData d4(10);
        TestData d5(50);
        TestData d6(20);
        TestData d7(80);

        list.pushBack(&d1);
        list.pushBack(&d2);
        list.pushBack(&d3);
        list.pushBack(&d4);
        list.pushBack(&d5);
        list.pushBack(&d6);
        list.pushBack(&d7);

        list.sort(ascendingComparator);

        IntrusiveNode* head = list.mutEnd();
        IntrusiveNode* current = head->next;
        int values[] = {10, 20, 30, 50, 70, 80, 90};
        int idx = 0;
        while (current != head) {
            TestData* data = static_cast<TestData*>(current);
            STD_INSIST(data->value == values[idx++]);
            current = current->next;
        }
        STD_INSIST(idx == 7);
    }

    STD_TEST(sortEvenNumberOfElements) {
        IntrusiveList list;
        TestData d1(60);
        TestData d2(20);
        TestData d3(80);
        TestData d4(10);
        TestData d5(50);
        TestData d6(30);

        list.pushBack(&d1);
        list.pushBack(&d2);
        list.pushBack(&d3);
        list.pushBack(&d4);
        list.pushBack(&d5);
        list.pushBack(&d6);

        list.sort(ascendingComparator);

        IntrusiveNode* head = list.mutEnd();
        IntrusiveNode* current = head->next;
        int values[] = {10, 20, 30, 50, 60, 80};
        int idx = 0;
        while (current != head) {
            TestData* data = static_cast<TestData*>(current);
            STD_INSIST(data->value == values[idx++]);
            current = current->next;
        }
        STD_INSIST(idx == 6);
    }

    STD_TEST(templateSortWithLambdaAscending) {
        IntrusiveList list;
        TestData d1(50);
        TestData d2(20);
        TestData d3(80);
        TestData d4(10);
        TestData d5(30);

        list.pushBack(&d1);
        list.pushBack(&d2);
        list.pushBack(&d3);
        list.pushBack(&d4);
        list.pushBack(&d5);

        list.sort([](const IntrusiveNode* a, const IntrusiveNode* b) {
            const TestData* ta = static_cast<const TestData*>(a);
            const TestData* tb = static_cast<const TestData*>(b);
            return ta->value < tb->value;
        });

        IntrusiveNode* head = list.mutEnd();
        IntrusiveNode* current = head->next;
        int values[] = {10, 20, 30, 50, 80};
        int idx = 0;
        while (current != head) {
            TestData* data = static_cast<TestData*>(current);
            STD_INSIST(data->value == values[idx++]);
            current = current->next;
        }
        STD_INSIST(idx == 5);
    }

    STD_TEST(templateSortWithLambdaDescending) {
        IntrusiveList list;
        TestData d1(10);
        TestData d2(50);
        TestData d3(30);
        TestData d4(20);
        TestData d5(40);

        list.pushBack(&d1);
        list.pushBack(&d2);
        list.pushBack(&d3);
        list.pushBack(&d4);
        list.pushBack(&d5);

        list.sort([](const IntrusiveNode* a, const IntrusiveNode* b) {
            const TestData* ta = static_cast<const TestData*>(a);
            const TestData* tb = static_cast<const TestData*>(b);
            return ta->value > tb->value;
        });

        IntrusiveNode* head = list.mutEnd();
        IntrusiveNode* current = head->next;
        int values[] = {50, 40, 30, 20, 10};
        int idx = 0;
        while (current != head) {
            TestData* data = static_cast<TestData*>(current);
            STD_INSIST(data->value == values[idx++]);
            current = current->next;
        }
        STD_INSIST(idx == 5);
    }

    STD_TEST(templateSortWithLambdaCapture) {
        IntrusiveList list;
        TestData d1(15);
        TestData d2(25);
        TestData d3(5);
        TestData d4(35);
        TestData d5(10);

        list.pushBack(&d1);
        list.pushBack(&d2);
        list.pushBack(&d3);
        list.pushBack(&d4);
        list.pushBack(&d5);

        int threshold = 20;
        list.sort([threshold](const IntrusiveNode* a, const IntrusiveNode* b) {
            const TestData* ta = static_cast<const TestData*>(a);
            const TestData* tb = static_cast<const TestData*>(b);
            int dist_a = (ta->value - threshold) * (ta->value - threshold);
            int dist_b = (tb->value - threshold) * (tb->value - threshold);
            return dist_a < dist_b;
        });

        IntrusiveNode* head = list.mutEnd();
        IntrusiveNode* current = head->next;
        int values[] = {25, 15, 10, 35, 5};
        int idx = 0;
        while (current != head) {
            TestData* data = static_cast<TestData*>(current);
            STD_INSIST(data->value == values[idx++]);
            current = current->next;
        }
        STD_INSIST(idx == 5);
    }

    struct AscendingFunctor {
        bool operator()(const IntrusiveNode* a, const IntrusiveNode* b) const {
            const TestData* ta = static_cast<const TestData*>(a);
            const TestData* tb = static_cast<const TestData*>(b);
            return ta->value < tb->value;
        }
    };

    struct DescendingFunctor {
        bool operator()(const IntrusiveNode* a, const IntrusiveNode* b) const {
            const TestData* ta = static_cast<const TestData*>(a);
            const TestData* tb = static_cast<const TestData*>(b);
            return ta->value > tb->value;
        }
    };

    STD_TEST(templateSortWithFunctor) {
        IntrusiveList list;
        TestData d1(40);
        TestData d2(10);
        TestData d3(30);
        TestData d4(20);
        TestData d5(50);

        list.pushBack(&d1);
        list.pushBack(&d2);
        list.pushBack(&d3);
        list.pushBack(&d4);
        list.pushBack(&d5);

        AscendingFunctor functor;
        list.sort(functor);

        IntrusiveNode* head = list.mutEnd();
        IntrusiveNode* current = head->next;
        int values[] = {10, 20, 30, 40, 50};
        int idx = 0;
        while (current != head) {
            TestData* data = static_cast<TestData*>(current);
            STD_INSIST(data->value == values[idx++]);
            current = current->next;
        }
        STD_INSIST(idx == 5);
    }

    STD_TEST(templateSortWithFunctorDescending) {
        IntrusiveList list;
        TestData d1(20);
        TestData d2(50);
        TestData d3(10);
        TestData d4(40);
        TestData d5(30);

        list.pushBack(&d1);
        list.pushBack(&d2);
        list.pushBack(&d3);
        list.pushBack(&d4);
        list.pushBack(&d5);

        DescendingFunctor functor;
        list.sort(functor);

        IntrusiveNode* head = list.mutEnd();
        IntrusiveNode* current = head->next;
        int values[] = {50, 40, 30, 20, 10};
        int idx = 0;
        while (current != head) {
            TestData* data = static_cast<TestData*>(current);
            STD_INSIST(data->value == values[idx++]);
            current = current->next;
        }
        STD_INSIST(idx == 5);
    }

    struct StatefulFunctor {
        mutable int callCount = 0;

        bool operator()(const IntrusiveNode* a, const IntrusiveNode* b) const {
            callCount++;
            const TestData* ta = static_cast<const TestData*>(a);
            const TestData* tb = static_cast<const TestData*>(b);
            return ta->value < tb->value;
        }
    };

    STD_TEST(templateSortWithStatefulFunctor) {
        IntrusiveList list;
        TestData d1(30);
        TestData d2(10);
        TestData d3(20);

        list.pushBack(&d1);
        list.pushBack(&d2);
        list.pushBack(&d3);

        StatefulFunctor functor;
        list.sort(functor);

        STD_INSIST(functor.callCount > 0);

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

    STD_TEST(templateSortEmptyList) {
        IntrusiveList list;
        list.sort([](const IntrusiveNode* a, const IntrusiveNode* b) {
            const TestData* ta = static_cast<const TestData*>(a);
            const TestData* tb = static_cast<const TestData*>(b);
            return ta->value < tb->value;
        });
        STD_INSIST(list.empty());
    }

    STD_TEST(templateSortSingleElement) {
        IntrusiveList list;
        TestData d1(42);
        list.pushBack(&d1);

        list.sort([](const IntrusiveNode* a, const IntrusiveNode* b) {
            const TestData* ta = static_cast<const TestData*>(a);
            const TestData* tb = static_cast<const TestData*>(b);
            return ta->value < tb->value;
        });

        STD_INSIST(list.mutFront() == &d1);
        STD_INSIST(list.mutBack() == &d1);
        TestData* data = static_cast<TestData*>(list.mutFront());
        STD_INSIST(data->value == 42);
    }

    STD_TEST(templateSortTwoElements) {
        IntrusiveList list;
        TestData d1(20);
        TestData d2(10);

        list.pushBack(&d1);
        list.pushBack(&d2);

        list.sort([](const IntrusiveNode* a, const IntrusiveNode* b) {
            const TestData* ta = static_cast<const TestData*>(a);
            const TestData* tb = static_cast<const TestData*>(b);
            return ta->value < tb->value;
        });

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

    STD_TEST(templateSortWithDuplicates) {
        IntrusiveList list;
        TestData d1(30);
        TestData d2(10);
        TestData d3(20);
        TestData d4(20);
        TestData d5(10);
        TestData d6(30);

        list.pushBack(&d1);
        list.pushBack(&d2);
        list.pushBack(&d3);
        list.pushBack(&d4);
        list.pushBack(&d5);
        list.pushBack(&d6);

        list.sort([](const IntrusiveNode* a, const IntrusiveNode* b) {
            const TestData* ta = static_cast<const TestData*>(a);
            const TestData* tb = static_cast<const TestData*>(b);
            return ta->value < tb->value;
        });

        IntrusiveNode* head = list.mutEnd();
        IntrusiveNode* current = head->next;
        int values[] = {10, 10, 20, 20, 30, 30};
        int idx = 0;
        while (current != head) {
            TestData* data = static_cast<TestData*>(current);
            STD_INSIST(data->value == values[idx++]);
            current = current->next;
        }
        STD_INSIST(idx == 6);
    }

    STD_TEST(templateSortBackwardTraversal) {
        IntrusiveList list;
        TestData d1(30);
        TestData d2(10);
        TestData d3(50);
        TestData d4(20);
        TestData d5(40);

        list.pushBack(&d1);
        list.pushBack(&d2);
        list.pushBack(&d3);
        list.pushBack(&d4);
        list.pushBack(&d5);

        list.sort([](const IntrusiveNode* a, const IntrusiveNode* b) {
            const TestData* ta = static_cast<const TestData*>(a);
            const TestData* tb = static_cast<const TestData*>(b);
            return ta->value < tb->value;
        });

        IntrusiveNode* head = list.mutEnd();
        IntrusiveNode* current = head->prev;
        int values[] = {50, 40, 30, 20, 10};
        int idx = 0;
        while (current != head) {
            TestData* data = static_cast<TestData*>(current);
            STD_INSIST(data->value == values[idx++]);
            current = current->prev;
        }
        STD_INSIST(idx == 5);
    }

    STD_TEST(templateSortMultipleTimes) {
        IntrusiveList list;
        TestData d1(30);
        TestData d2(10);
        TestData d3(20);

        list.pushBack(&d1);
        list.pushBack(&d2);
        list.pushBack(&d3);

        list.sort([](const IntrusiveNode* a, const IntrusiveNode* b) {
            const TestData* ta = static_cast<const TestData*>(a);
            const TestData* tb = static_cast<const TestData*>(b);
            return ta->value < tb->value;
        });

        IntrusiveNode* head = list.mutEnd();
        IntrusiveNode* current = head->next;
        int values1[] = {10, 20, 30};
        int idx1 = 0;
        while (current != head) {
            TestData* data = static_cast<TestData*>(current);
            STD_INSIST(data->value == values1[idx1++]);
            current = current->next;
        }
        STD_INSIST(idx1 == 3);

        list.sort([](const IntrusiveNode* a, const IntrusiveNode* b) {
            const TestData* ta = static_cast<const TestData*>(a);
            const TestData* tb = static_cast<const TestData*>(b);
            return ta->value > tb->value;
        });

        head = list.mutEnd();
        current = head->next;
        int values2[] = {30, 20, 10};
        int idx2 = 0;
        while (current != head) {
            TestData* data = static_cast<TestData*>(current);
            STD_INSIST(data->value == values2[idx2++]);
            current = current->next;
        }
        STD_INSIST(idx2 == 3);
    }

    STD_TEST(templateSortLargeList) {
        IntrusiveList list;
        TestData d1(100);
        TestData d2(25);
        TestData d3(75);
        TestData d4(10);
        TestData d5(90);
        TestData d6(5);
        TestData d7(50);
        TestData d8(30);
        TestData d9(60);
        TestData d10(80);

        list.pushBack(&d1);
        list.pushBack(&d2);
        list.pushBack(&d3);
        list.pushBack(&d4);
        list.pushBack(&d5);
        list.pushBack(&d6);
        list.pushBack(&d7);
        list.pushBack(&d8);
        list.pushBack(&d9);
        list.pushBack(&d10);

        list.sort([](const IntrusiveNode* a, const IntrusiveNode* b) {
            const TestData* ta = static_cast<const TestData*>(a);
            const TestData* tb = static_cast<const TestData*>(b);
            return ta->value < tb->value;
        });

        IntrusiveNode* head = list.mutEnd();
        IntrusiveNode* current = head->next;
        int values[] = {5, 10, 25, 30, 50, 60, 75, 80, 90, 100};
        int idx = 0;
        while (current != head) {
            TestData* data = static_cast<TestData*>(current);
            STD_INSIST(data->value == values[idx++]);
            current = current->next;
        }
        STD_INSIST(idx == 10);
    }

    STD_TEST(templateSortPreservesLength) {
        IntrusiveList list;
        TestData d1(50);
        TestData d2(20);
        TestData d3(80);
        TestData d4(10);
        TestData d5(30);

        list.pushBack(&d1);
        list.pushBack(&d2);
        list.pushBack(&d3);
        list.pushBack(&d4);
        list.pushBack(&d5);

        unsigned lengthBefore = list.length();

        list.sort([](const IntrusiveNode* a, const IntrusiveNode* b) {
            const TestData* ta = static_cast<const TestData*>(a);
            const TestData* tb = static_cast<const TestData*>(b);
            return ta->value < tb->value;
        });

        unsigned lengthAfter = list.length();

        STD_INSIST(lengthBefore == 5);
        STD_INSIST(lengthAfter == 5);
    }

    STD_TEST(templateSortWithMutableLambda) {
        IntrusiveList list;
        TestData d1(30);
        TestData d2(10);
        TestData d3(20);
        TestData d4(40);

        list.pushBack(&d1);
        list.pushBack(&d2);
        list.pushBack(&d3);
        list.pushBack(&d4);

        int comparisonCount = 0;
        list.sort([&comparisonCount](const IntrusiveNode* a, const IntrusiveNode* b) {
            comparisonCount++;
            const TestData* ta = static_cast<const TestData*>(a);
            const TestData* tb = static_cast<const TestData*>(b);
            return ta->value < tb->value;
        });

        STD_INSIST(comparisonCount > 0);

        IntrusiveNode* head = list.mutEnd();
        IntrusiveNode* current = head->next;
        int values[] = {10, 20, 30, 40};
        int idx = 0;
        while (current != head) {
            TestData* data = static_cast<TestData*>(current);
            STD_INSIST(data->value == values[idx++]);
            current = current->next;
        }
        STD_INSIST(idx == 4);
    }

    STD_TEST(templateSortFrontAndBackPointers) {
        IntrusiveList list;
        TestData d1(40);
        TestData d2(10);
        TestData d3(30);
        TestData d4(20);

        list.pushBack(&d1);
        list.pushBack(&d2);
        list.pushBack(&d3);
        list.pushBack(&d4);

        list.sort([](const IntrusiveNode* a, const IntrusiveNode* b) {
            const TestData* ta = static_cast<const TestData*>(a);
            const TestData* tb = static_cast<const TestData*>(b);
            return ta->value < tb->value;
        });

        TestData* front = static_cast<TestData*>(list.mutFront());
        TestData* back = static_cast<TestData*>(list.mutBack());

        STD_INSIST(front->value == 10);
        STD_INSIST(back->value == 40);
    }
}
