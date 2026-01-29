#include "smap.h"

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
}
