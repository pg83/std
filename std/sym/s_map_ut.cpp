#include "s_map.h"

#include <std/tst/ut.h>
#include <std/lib/vector.h>

#include <stdio.h>

using namespace Std;

STD_TEST_SUITE(SymbolMap) {
    STD_TEST(test1) {
        SymbolMap<int> s;

        s["qw1"] = 1;
        s["qw2"] = 2;

        STD_INSIST(s["qw1"] == 1);
        STD_INSIST(s["qw2"] == 2);
        STD_INSIST(s["qw3"] == 0);
    }

    STD_TEST(test2) {
        SymbolMap<Vector<int>> s;

        s["qw1"].pushBack(1);
        s["qw1"].pushBack(2);
        s["qw2"].pushBack(3);
        s["qw2"].pushBack(4);

        STD_INSIST(s["qw1"][0] == 1);
        STD_INSIST(s["qw1"][1] == 2);
        STD_INSIST(s["qw2"][0] == 3);
        STD_INSIST(s["qw2"][1] == 4);
    }

    STD_TEST(DefaultConstructor) {
        SymbolMap<int> map;

        STD_INSIST(map.find("nonexistent") == nullptr);
    }

    STD_TEST(FindNonExistent) {
        SymbolMap<int> map;

        int* result = map.find("key");

        STD_INSIST(result == nullptr);
    }

    STD_TEST(FindExisting) {
        SymbolMap<int> map;
        map.insert("key", 42);

        int* result = map.find("key");

        STD_INSIST(result != nullptr);
        STD_INSIST(*result == 42);
    }

    STD_TEST(FindEmptyKey) {
        SymbolMap<int> map;
        map.insert("", 100);

        int* result = map.find("");

        STD_INSIST(result != nullptr);
        STD_INSIST(*result == 100);
    }

    STD_TEST(FindAfterMultipleInserts) {
        SymbolMap<int> map;

        map.insert("a", 1);
        map.insert("b", 2);
        map.insert("c", 3);

        STD_INSIST(*map.find("a") == 1);
        STD_INSIST(*map.find("b") == 2);
        STD_INSIST(*map.find("c") == 3);
    }

    STD_TEST(FindConstCorrectness) {
        SymbolMap<int> map;
        map.insert("key", 42);

        const SymbolMap<int>& constMap = map;
        int* result = constMap.find("key");

        STD_INSIST(result != nullptr);
        STD_INSIST(*result == 42);
    }

    STD_TEST(InsertBasic) {
        SymbolMap<int> map;

        int* result = map.insert("key", 42);

        STD_INSIST(result != nullptr);
        STD_INSIST(*result == 42);
    }

    STD_TEST(InsertReturnsPointerToValue) {
        SymbolMap<int> map;

        int* inserted = map.insert("key", 42);
        int* found = map.find("key");

        STD_INSIST(inserted == found);
    }

    STD_TEST(InsertDefaultConstructed) {
        SymbolMap<int> map;

        int* result = map.insert("key");

        STD_INSIST(result != nullptr);
        STD_INSIST(*result == 0); // int default-initialized
    }

    STD_TEST(InsertWithMultipleArgs) {
        struct Point {
            int x, y;
            Point(int x_, int y_)
                : x(x_)
                , y(y_)
            {
            }
        };

        SymbolMap<Point> map;

        Point* result = map.insert("origin", 10, 20);

        STD_INSIST(result != nullptr);
        STD_INSIST(result->x == 10);
        STD_INSIST(result->y == 20);
    }

    STD_TEST(InsertOverwritesExisting) {
        SymbolMap<int> map;
        map.insert("key", 1);

        map.insert("key", 2);

        int* result = map.find("key");
        STD_INSIST(result != nullptr);
        STD_INSIST(*result == 2);
    }

    STD_TEST(InsertEmptyKey) {
        SymbolMap<int> map;

        int* result = map.insert("", 42);

        STD_INSIST(result != nullptr);
        STD_INSIST(*result == 42);
    }

    STD_TEST(InsertLongKey) {
        SymbolMap<int> map;
        StringView longKey = "this_is_a_very_long_key_that_should_still_work_correctly";

        int* result = map.insert(longKey, 42);

        STD_INSIST(result != nullptr);
        STD_INSIST(*result == 42);
        STD_INSIST(*map.find(longKey) == 42);
    }

    STD_TEST(InsertManyKeys) {
        SymbolMap<int> map;

        for (int i = 0; i < 100; ++i) {
            char key[16];
            snprintf(key, sizeof(key), "key%d", i);
            map.insert((const char*)key, i);
        }

        for (int i = 0; i < 100; ++i) {
            char key[16];
            snprintf(key, sizeof(key), "key%d", i);
            int* result = map.find((const char*)key);
            STD_INSIST(result != nullptr);
            STD_INSIST(*result == i);
        }
    }

    STD_TEST(OperatorBracketCreatesNew) {
        SymbolMap<int> map;

        int& value = map["newkey"];

        STD_INSIST(map.find("newkey") != nullptr);
    }

    STD_TEST(OperatorBracketReturnsExisting) {
        SymbolMap<int> map;
        map.insert("key", 42);

        int& value = map["key"];

        STD_INSIST(value == 42);
    }

    STD_TEST(OperatorBracketModifiable) {
        SymbolMap<int> map;
        map.insert("key", 10);

        map["key"] = 20;

        STD_INSIST(*map.find("key") == 20);
    }

    STD_TEST(OperatorBracketSamePointer) {
        SymbolMap<int> map;
        map.insert("key", 42);

        int* found = map.find("key");
        int& ref = map["key"];

        STD_INSIST(&ref == found);
    }

    STD_TEST(OperatorBracketNewValueModifiable) {
        SymbolMap<int> map;

        map["key"] = 100;

        STD_INSIST(*map.find("key") == 100);
    }

    STD_TEST(OperatorBracketChained) {
        SymbolMap<int> map;

        map["a"] = 1;
        map["b"] = 2;
        map["c"] = map["a"] + map["b"];

        STD_INSIST(map["c"] == 3);
    }

    STD_TEST(StringValue) {
        SymbolMap<StringView> map;

        map.insert("greeting", "hello");

        StringView* result = map.find("greeting");
        STD_INSIST(result != nullptr);
        STD_INSIST(*result == StringView("hello"));
    }

    STD_TEST(StructValue) {
        struct Data {
            int id;
            float value;

            Data()
                : id(0)
                , value(0.0f)
            {
            }
            Data(int i, float v)
                : id(i)
                , value(v)
            {
            }
        };

        SymbolMap<Data> map;
        map.insert("item", 42, 3.14f);

        Data* result = map.find("item");
        STD_INSIST(result != nullptr);
        STD_INSIST(result->id == 42);
        STD_INSIST(result->value == 3.14f);
    }

    STD_TEST(PointerValue) {
        int x = 42;
        SymbolMap<int*> map;

        map.insert("ptr", &x);

        int** result = map.find("ptr");
        STD_INSIST(result != nullptr);
        STD_INSIST(*result == &x);
        STD_INSIST(**result == 42);
    }

    STD_TEST(SimilarKeys) {
        SymbolMap<int> map;
        map.insert("key", 1);
        map.insert("key1", 2);
        map.insert("key12", 3);
        map.insert("key123", 4);

        STD_INSIST(*map.find("key") == 1);
        STD_INSIST(*map.find("key1") == 2);
        STD_INSIST(*map.find("key12") == 3);
        STD_INSIST(*map.find("key123") == 4);
    }

    STD_TEST(KeysWithSpecialChars) {
        SymbolMap<int> map;

        map.insert("key with spaces", 1);
        map.insert("key\twith\ttabs", 2);
        map.insert("key.with.dots", 3);
        map.insert("key/with/slashes", 4);

        STD_INSIST(*map.find("key with spaces") == 1);
        STD_INSIST(*map.find("key\twith\ttabs") == 2);
        STD_INSIST(*map.find("key.with.dots") == 3);
        STD_INSIST(*map.find("key/with/slashes") == 4);
    }

    STD_TEST(NumericStringKeys) {
        SymbolMap<int> map;

        map.insert("0", 0);
        map.insert("1", 1);
        map.insert("123", 123);
        map.insert("-1", -1);

        STD_INSIST(*map.find("0") == 0);
        STD_INSIST(*map.find("1") == 1);
        STD_INSIST(*map.find("123") == 123);
        STD_INSIST(*map.find("-1") == -1);
    }

    STD_TEST(ModifyThroughFind) {
        SymbolMap<int> map;
        map.insert("key", 10);

        int* ptr = map.find("key");
        *ptr = 20;

        STD_INSIST(*map.find("key") == 20);
    }

    STD_TEST(MultipleMapInstances) {
        SymbolMap<int> map1;
        SymbolMap<int> map2;

        map1.insert("key", 1);
        map2.insert("key", 2);

        STD_INSIST(*map1.find("key") == 1);
        STD_INSIST(*map2.find("key") == 2);
    }

    STD_TEST(InsertWithMove) {
        struct MoveOnly {
            int value;
            bool moved = false;

            MoveOnly(int v)
                : value(v)
            {
            }

            MoveOnly(MoveOnly&& other)
                : value(other.value)
                , moved(true)
            {
                other.value = 0;
            }
            MoveOnly(const MoveOnly&) = delete;
        };

        SymbolMap<MoveOnly> map;

        MoveOnly* result = map.insert("key", 42);

        STD_INSIST(result != nullptr);
        STD_INSIST(result->value == 42);
    }

    STD_TEST(Compactify) {
        SymbolMap<int> s;

        s["a"] = 1;
        s["b"] = 2;

        s.compactify();

        STD_INSIST(s["a"] == 1);
        STD_INSIST(s["b"] == 2);
    }

    STD_TEST(CompactifyEmpty) {
        SymbolMap<int> s;

        s.compactify();

        STD_INSIST(s.find("key") == nullptr);
    }

    STD_TEST(CompactifyManyElements) {
        SymbolMap<int> s;

        for (int i = 0; i < 100; ++i) {
            char key[16];
            snprintf(key, sizeof(key), "key%d", i);
            s[(const char*)key] = i * 10;
        }

        s.compactify();

        for (int i = 0; i < 100; ++i) {
            char key[16];
            snprintf(key, sizeof(key), "key%d", i);
            STD_INSIST(s[(const char*)key] == i * 10);
        }
    }

    STD_TEST(CompactifyPreservesValues) {
        SymbolMap<int> s;

        s["first"] = 100;
        s["second"] = 200;
        s["third"] = 300;

        s.compactify();

        STD_INSIST(*s.find("first") == 100);
        STD_INSIST(*s.find("second") == 200);
        STD_INSIST(*s.find("third") == 300);
    }

    STD_TEST(CompactifyComplexType) {
        struct Data {
            int id;
            float value;

            Data()
                : id(0)
                , value(0.0f)
            {
            }
            Data(int i, float v)
                : id(i)
                , value(v)
            {
            }
        };

        SymbolMap<Data> s;

        s.insert("item1", 1, 1.5f);
        s.insert("item2", 2, 2.5f);
        s.insert("item3", 3, 3.5f);

        s.compactify();

        Data* d1 = s.find("item1");
        Data* d2 = s.find("item2");
        Data* d3 = s.find("item3");

        STD_INSIST(d1 != nullptr);
        STD_INSIST(d1->id == 1);
        STD_INSIST(d1->value == 1.5f);

        STD_INSIST(d2 != nullptr);
        STD_INSIST(d2->id == 2);
        STD_INSIST(d2->value == 2.5f);

        STD_INSIST(d3 != nullptr);
        STD_INSIST(d3->id == 3);
        STD_INSIST(d3->value == 3.5f);
    }

    STD_TEST(CompactifyMultipleTimes) {
        SymbolMap<int> s;

        s["a"] = 1;
        s["b"] = 2;

        s.compactify();

        STD_INSIST(s["a"] == 1);
        STD_INSIST(s["b"] == 2);

        s.compactify();

        STD_INSIST(s["a"] == 1);
        STD_INSIST(s["b"] == 2);

        s.compactify();

        STD_INSIST(s["a"] == 1);
        STD_INSIST(s["b"] == 2);
    }

    STD_TEST(CompactifyThenInsert) {
        SymbolMap<int> s;

        s["before"] = 10;

        s.compactify();

        s["after"] = 20;

        STD_INSIST(s["before"] == 10);
        STD_INSIST(s["after"] == 20);
    }

    STD_TEST(CompactifyThenModify) {
        SymbolMap<int> s;

        s["key"] = 100;

        s.compactify();

        s["key"] = 200;

        STD_INSIST(*s.find("key") == 200);
    }

    STD_TEST(CompactifyWithVectorValues) {
        SymbolMap<Vector<int>> s;

        s["v1"].pushBack(1);
        s["v1"].pushBack(2);

        s["v2"].pushBack(3);
        s["v2"].pushBack(4);
        s["v2"].pushBack(5);

        STD_INSIST(s["v1"].length() == 2);
        STD_INSIST(s["v1"][0] == 1);
        STD_INSIST(s["v1"][1] == 2);

        STD_INSIST(s["v2"].length() == 3);
        STD_INSIST(s["v2"][0] == 3);
        STD_INSIST(s["v2"][1] == 4);
        STD_INSIST(s["v2"][2] == 5);

        s.compactify();

        STD_INSIST(s.find("v1") != nullptr);
        STD_INSIST(s.find("v2") != nullptr);

        STD_INSIST(s["v1"].length() == 2);
        STD_INSIST(s["v1"][0] == 1);
        STD_INSIST(s["v1"][1] == 2);

        STD_INSIST(s["v2"].length() == 3);
        STD_INSIST(s["v2"][0] == 3);
        STD_INSIST(s["v2"][1] == 4);
        STD_INSIST(s["v2"][2] == 5);
    }

    STD_TEST(CompactifyWithEmptyStringKey) {
        SymbolMap<int> s;

        s[""] = 999;
        s["normal"] = 111;

        s.compactify();

        STD_INSIST(*s.find("") == 999);
        STD_INSIST(*s.find("normal") == 111);
    }

    STD_TEST(CompactifyPreservesAllKeys) {
        SymbolMap<int> s;

        s["alpha"] = 1;
        s["beta"] = 2;
        s["gamma"] = 3;
        s["delta"] = 4;
        s["epsilon"] = 5;

        s.compactify();

        STD_INSIST(s.find("alpha") != nullptr);
        STD_INSIST(s.find("beta") != nullptr);
        STD_INSIST(s.find("gamma") != nullptr);
        STD_INSIST(s.find("delta") != nullptr);
        STD_INSIST(s.find("epsilon") != nullptr);

        STD_INSIST(*s.find("alpha") == 1);
        STD_INSIST(*s.find("beta") == 2);
        STD_INSIST(*s.find("gamma") == 3);
        STD_INSIST(*s.find("delta") == 4);
        STD_INSIST(*s.find("epsilon") == 5);
    }

    STD_TEST(CompactifySingleElement) {
        SymbolMap<int> s;

        s["only"] = 42;

        s.compactify();

        STD_INSIST(*s.find("only") == 42);
    }

    STD_TEST(CompactifyLargeValues) {
        struct LargeStruct {
            int data[100];

            LargeStruct() {
                for (int i = 0; i < 100; ++i) {
                    data[i] = i;
                }
            }
        };

        SymbolMap<LargeStruct> s;

        s.insert("large1");
        s.insert("large2");

        s.compactify();

        LargeStruct* ls1 = s.find("large1");
        LargeStruct* ls2 = s.find("large2");

        STD_INSIST(ls1 != nullptr);
        STD_INSIST(ls2 != nullptr);

        for (int i = 0; i < 100; ++i) {
            STD_INSIST(ls1->data[i] == i);
            STD_INSIST(ls2->data[i] == i);
        }
    }
}
