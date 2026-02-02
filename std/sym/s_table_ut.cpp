#include "s_table.h"

#include <std/tst/ut.h>
#include <std/str/view.h>

#include <stdio.h>

using namespace Std;

STD_TEST_SUITE(SymbolTable) {
    STD_TEST(BasicSetAndFind) {
        SymbolTable s;

        s.set("qw1", (void*)2);
        s.set("qw2", (void*)3);

        STD_INSIST(s.find("qw1") == (void*)2);
        STD_INSIST(s.find("qw2") == (void*)3);
        STD_INSIST(s.find("qw3") == nullptr);
    }

    STD_TEST(EmptyTable) {
        SymbolTable s;

        STD_INSIST(s.find("any") == nullptr);
        STD_INSIST(s.find("") == nullptr);
    }

    STD_TEST(SingleElement) {
        SymbolTable s;

        int value = 42;
        s.set("single", &value);

        STD_INSIST(s.find("single") == &value);
        STD_INSIST(s.find("other") == nullptr);
    }

    STD_TEST(UpdateExisting) {
        SymbolTable s;

        int value1 = 100;
        int value2 = 200;

        s.set("key", &value1);
        STD_INSIST(s.find("key") == &value1);

        s.set("key", &value2);
        STD_INSIST(s.find("key") == &value2);
    }

    STD_TEST(NullptrValue) {
        SymbolTable s;

        s.set("nullkey", nullptr);

        STD_INSIST(s.find("nullkey") == nullptr);
        STD_INSIST(s.find("other") == nullptr);
    }

    STD_TEST(EmptyStringKey) {
        SymbolTable s;

        int value = 999;
        s.set("", &value);

        STD_INSIST(s.find("") == &value);
        STD_INSIST(s.find("nonempty") == nullptr);
    }

    STD_TEST(LongKey) {
        SymbolTable s;

        StringView longKey = "this_is_a_very_long_key_name_that_should_still_work_correctly_in_symbol_table";
        int value = 12345;

        s.set(longKey, &value);

        STD_INSIST(s.find(longKey) == &value);
    }

    STD_TEST(SimilarKeys) {
        SymbolTable s;

        int v1 = 1, v2 = 2, v3 = 3, v4 = 4;

        s.set("key", &v1);
        s.set("key1", &v2);
        s.set("key12", &v3);
        s.set("key123", &v4);

        STD_INSIST(s.find("key") == &v1);
        STD_INSIST(s.find("key1") == &v2);
        STD_INSIST(s.find("key12") == &v3);
        STD_INSIST(s.find("key123") == &v4);
    }

    STD_TEST(SpecialCharacters) {
        SymbolTable s;

        int v1 = 1, v2 = 2, v3 = 3, v4 = 4;

        s.set("key with spaces", &v1);
        s.set("key\twith\ttabs", &v2);
        s.set("key.with.dots", &v3);
        s.set("key/with/slashes", &v4);

        STD_INSIST(s.find("key with spaces") == &v1);
        STD_INSIST(s.find("key\twith\ttabs") == &v2);
        STD_INSIST(s.find("key.with.dots") == &v3);
        STD_INSIST(s.find("key/with/slashes") == &v4);
    }

    STD_TEST(NumericStringKeys) {
        SymbolTable s;

        int v0 = 0, v1 = 1, v123 = 123, vminus = -1;

        s.set("0", &v0);
        s.set("1", &v1);
        s.set("123", &v123);
        s.set("-1", &vminus);

        STD_INSIST(s.find("0") == &v0);
        STD_INSIST(s.find("1") == &v1);
        STD_INSIST(s.find("123") == &v123);
        STD_INSIST(s.find("-1") == &vminus);
    }

    STD_TEST(ManyElements) {
        SymbolTable s;

        const int count = 1000;
        int* values = new int[count];

        for (int i = 0; i < count; ++i) {
            values[i] = i * 7;
            char key[32];
            snprintf(key, sizeof(key), "key%d", i);
            s.set((const char*)key, &values[i]);
        }

        for (int i = 0; i < count; ++i) {
            char key[32];
            snprintf(key, sizeof(key), "key%d", i);
            int* found = static_cast<int*>(s.find((const char*)key));
            STD_INSIST(found != nullptr);
            STD_INSIST(*found == i * 7);
        }

        delete[] values;
    }

    STD_TEST(PointerValues) {
        SymbolTable s;

        int a = 1, b = 2, c = 3;
        int* ptrA = &a;
        int* ptrB = &b;
        int* ptrC = &c;

        s.set("ptrA", &ptrA);
        s.set("ptrB", &ptrB);
        s.set("ptrC", &ptrC);

        int** foundA = static_cast<int**>(s.find("ptrA"));
        int** foundB = static_cast<int**>(s.find("ptrB"));
        int** foundC = static_cast<int**>(s.find("ptrC"));

        STD_INSIST(foundA != nullptr && *foundA == &a);
        STD_INSIST(foundB != nullptr && *foundB == &b);
        STD_INSIST(foundC != nullptr && *foundC == &c);
    }

    STD_TEST(StructValues) {
        struct Data {
            int x;
            double y;
            const char* name;
        };

        SymbolTable s;

        Data d1 = {10, 1.5, "first"};
        Data d2 = {20, 2.5, "second"};
        Data d3 = {30, 3.5, "third"};

        s.set("data1", &d1);
        s.set("data2", &d2);
        s.set("data3", &d3);

        Data* found1 = static_cast<Data*>(s.find("data1"));
        Data* found2 = static_cast<Data*>(s.find("data2"));
        Data* found3 = static_cast<Data*>(s.find("data3"));

        STD_INSIST(found1 != nullptr);
        STD_INSIST(found1->x == 10);
        STD_INSIST(found1->y == 1.5);

        STD_INSIST(found2 != nullptr);
        STD_INSIST(found2->x == 20);

        STD_INSIST(found3 != nullptr);
        STD_INSIST(found3->x == 30);
    }

    STD_TEST(MixedOperations) {
        SymbolTable s;

        int values[10];
        for (int i = 0; i < 10; ++i) {
            values[i] = i * 10;
        }

        for (int i = 0; i < 10; i += 2) {
            char key[16];
            snprintf(key, sizeof(key), "k%d", i);
            s.set((const char*)key, &values[i]);
        }

        for (int i = 1; i < 10; i += 2) {
            char key[16];
            snprintf(key, sizeof(key), "k%d", i);
            s.set((const char*)key, &values[i]);
        }

        for (int i = 0; i < 10; ++i) {
            char key[16];
            snprintf(key, sizeof(key), "k%d", i);
            int* found = static_cast<int*>(s.find((const char*)key));
            STD_INSIST(found != nullptr);
            STD_INSIST(*found == i * 10);
        }
    }

    STD_TEST(ConstCorrectness) {
        SymbolTable s;

        int value = 42;
        s.set("key", &value);

        const SymbolTable& constTable = s;
        void* result = constTable.find("key");

        STD_INSIST(result == &value);
    }

    STD_TEST(FindNonExistentKeys) {
        SymbolTable s;

        int value = 100;
        s.set("exists", &value);

        STD_INSIST(s.find("notexist") == nullptr);
        STD_INSIST(s.find("exist") == nullptr);
        STD_INSIST(s.find("existss") == nullptr);
        STD_INSIST(s.find("") == nullptr);
    }

    STD_TEST(OverwriteMultipleTimes) {
        SymbolTable s;

        int v1 = 1, v2 = 2, v3 = 3, v4 = 4;

        s.set("key", &v1);
        STD_INSIST(s.find("key") == &v1);

        s.set("key", &v2);
        STD_INSIST(s.find("key") == &v2);

        s.set("key", &v3);
        STD_INSIST(s.find("key") == &v3);

        s.set("key", &v4);
        STD_INSIST(s.find("key") == &v4);
    }

    STD_TEST(StringViewKeys) {
        SymbolTable s;

        int v1 = 100, v2 = 200;

        StringView sv1("first_key");
        StringView sv2("second_key");

        s.set(sv1, &v1);
        s.set(sv2, &v2);

        STD_INSIST(s.find(sv1) == &v1);
        STD_INSIST(s.find(sv2) == &v2);
        STD_INSIST(s.find("first_key") == &v1);
        STD_INSIST(s.find("second_key") == &v2);
    }

    STD_TEST(CaseSensitiveKeys) {
        SymbolTable s;

        int v1 = 1, v2 = 2, v3 = 3;

        s.set("Key", &v1);
        s.set("KEY", &v2);
        s.set("key", &v3);

        STD_INSIST(s.find("Key") == &v1);
        STD_INSIST(s.find("KEY") == &v2);
        STD_INSIST(s.find("key") == &v3);
    }

    STD_TEST(ForEachEmpty) {
        SymbolTable s;

        struct CountIterator: HashTable::Iterator {
            int count = 0;
            void process(void** el) override {
                count++;
            }
        };

        CountIterator it;
        s.forEach(it);

        STD_INSIST(it.count == 0);
    }

    STD_TEST(ForEachSingle) {
        SymbolTable s;

        int value = 42;
        s.set("key", &value);

        struct CountIterator: HashTable::Iterator {
            int count = 0;
            void** lastEl = nullptr;
            void process(void** el) override {
                count++;
                lastEl = el;
            }
        };

        CountIterator it;
        s.forEach(it);

        STD_INSIST(it.count == 1);
        STD_INSIST(it.lastEl != nullptr);
        STD_INSIST(*it.lastEl == &value);
    }

    STD_TEST(ForEachMultiple) {
        SymbolTable s;

        int v1 = 10, v2 = 20, v3 = 30, v4 = 40, v5 = 50;

        s.set("k1", &v1);
        s.set("k2", &v2);
        s.set("k3", &v3);
        s.set("k4", &v4);
        s.set("k5", &v5);

        struct CountIterator: HashTable::Iterator {
            int count = 0;
            void process(void** el) override {
                count++;
            }
        };

        CountIterator it;
        s.forEach(it);

        STD_INSIST(it.count == 5);
    }

    STD_TEST(ForEachSumValues) {
        SymbolTable s;

        int values[10];
        for (int i = 0; i < 10; ++i) {
            values[i] = i * 10;
            char key[16];
            snprintf(key, sizeof(key), "k%d", i);
            s.set((const char*)key, &values[i]);
        }

        struct SumIterator: HashTable::Iterator {
            int sum = 0;
            void process(void** el) override {
                int* ptr = static_cast<int*>(*el);
                sum += *ptr;
            }
        };

        SumIterator it;
        s.forEach(it);

        STD_INSIST(it.sum == 0 + 10 + 20 + 30 + 40 + 50 + 60 + 70 + 80 + 90);
    }

    STD_TEST(ForEachModifyValues) {
        SymbolTable s;

        int values[5] = {1, 2, 3, 4, 5};
        for (int i = 0; i < 5; ++i) {
            char key[16];
            snprintf(key, sizeof(key), "k%d", i);
            s.set((const char*)key, &values[i]);
        }

        struct MultiplyIterator: HashTable::Iterator {
            void process(void** el) override {
                int* ptr = static_cast<int*>(*el);
                *ptr *= 2;
            }
        };

        MultiplyIterator it;
        s.forEach(it);

        for (int i = 0; i < 5; ++i) {
            STD_INSIST(values[i] == (i + 1) * 2);
        }
    }

    STD_TEST(ForEachCollectPointers) {
        SymbolTable s;

        int v1 = 10, v2 = 20, v3 = 30;

        s.set("a", &v1);
        s.set("b", &v2);
        s.set("c", &v3);

        struct CollectIterator: HashTable::Iterator {
            void* collected[3];
            int index = 0;
            void process(void** el) override {
                if (index < 3) {
                    collected[index++] = *el;
                }
            }
        };

        CollectIterator it;
        s.forEach(it);

        STD_INSIST(it.index == 3);

        bool found[3] = {false, false, false};
        for (int i = 0; i < 3; ++i) {
            if (it.collected[i] == &v1) {
                found[0] = true;
            }
            if (it.collected[i] == &v2) {
                found[1] = true;
            }
            if (it.collected[i] == &v3) {
                found[2] = true;
            }
        }

        STD_INSIST(found[0] && found[1] && found[2]);
    }

    STD_TEST(MultipleInstances) {
        SymbolTable s1;
        SymbolTable s2;

        int v1 = 100, v2 = 200;

        s1.set("key", &v1);
        s2.set("key", &v2);

        STD_INSIST(s1.find("key") == &v1);
        STD_INSIST(s2.find("key") == &v2);
    }

    STD_TEST(UnicodeKeys) {
        SymbolTable s;

        int v1 = 1, v2 = 2, v3 = 3;

        s.set("привет", &v1);
        s.set("世界", &v2);
        s.set("🌍", &v3);

        STD_INSIST(s.find("привет") == &v1);
        STD_INSIST(s.find("世界") == &v2);
        STD_INSIST(s.find("🌍") == &v3);
    }

    STD_TEST(HashCollisions) {
        SymbolTable s;

        int values[100];
        for (int i = 0; i < 100; ++i) {
            values[i] = i;
            char key[32];
            snprintf(key, sizeof(key), "collision_test_%d", i);
            s.set((const char*)key, &values[i]);
        }

        for (int i = 0; i < 100; ++i) {
            char key[32];
            snprintf(key, sizeof(key), "collision_test_%d", i);
            int* found = static_cast<int*>(s.find((const char*)key));
            STD_INSIST(found == &values[i]);
            STD_INSIST(*found == i);
        }
    }

    STD_TEST(PrefixKeys) {
        SymbolTable s;

        int v1 = 1, v2 = 2, v3 = 3, v4 = 4;

        s.set("a", &v1);
        s.set("ab", &v2);
        s.set("abc", &v3);
        s.set("abcd", &v4);

        STD_INSIST(s.find("a") == &v1);
        STD_INSIST(s.find("ab") == &v2);
        STD_INSIST(s.find("abc") == &v3);
        STD_INSIST(s.find("abcd") == &v4);
    }

    STD_TEST(ReverseInsertionOrder) {
        SymbolTable s;

        int values[10];
        for (int i = 9; i >= 0; --i) {
            values[i] = i * 100;
            char key[16];
            snprintf(key, sizeof(key), "k%d", i);
            s.set((const char*)key, &values[i]);
        }

        for (int i = 0; i < 10; ++i) {
            char key[16];
            snprintf(key, sizeof(key), "k%d", i);
            int* found = static_cast<int*>(s.find((const char*)key));
            STD_INSIST(found == &values[i]);
            STD_INSIST(*found == i * 100);
        }
    }
}
