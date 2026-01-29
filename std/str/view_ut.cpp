#include "view.h"
#include "dynamic.h"

#include <std/tst/ut.h>

using namespace Std;

STD_TEST_SUITE(StringView) {
    STD_TEST(DefaultConstruction) {
        StringView sv;
        STD_INSIST(sv.empty());
        STD_INSIST(sv.length() == 0);
        STD_INSIST(sv.data() == nullptr);
    }

    STD_TEST(ConstructionFromCString) {
        StringView sv("hello");
        STD_INSIST(!sv.empty());
        STD_INSIST(sv.length() == 5);
        STD_INSIST(sv.data() != nullptr);
    }

    STD_TEST(ConstructionFromEmptyCString) {
        StringView sv("");
        STD_INSIST(sv.empty());
        STD_INSIST(sv.length() == 0);
    }

    STD_TEST(ConstructionFromPtrAndLength) {
        const u8* data = (const u8*)"test";
        StringView sv(data, 4);
        STD_INSIST(sv.length() == 4);
        STD_INSIST(sv.data() == data);
    }

    STD_TEST(ConstructionFromZeroLength) {
        const u8* data = (const u8*)"test";
        StringView sv(data, 0);
        STD_INSIST(sv.empty());
        STD_INSIST(sv.length() == 0);
    }

    STD_TEST(ConstructionFromArray) {
        const u8 arr[] = u8"array";
        StringView sv(arr);
        STD_INSIST(sv.length() == 5);
    }

    STD_TEST(CopyConstruction) {
        StringView sv1("original");
        StringView sv2(sv1);
        STD_INSIST(sv1.length() == sv2.length());
        STD_INSIST(sv1.data() == sv2.data());
    }

    STD_TEST(IndexOperator) {
        StringView sv("hello");
        STD_INSIST(sv[0] == 'h');
        STD_INSIST(sv[1] == 'e');
        STD_INSIST(sv[2] == 'l');
        STD_INSIST(sv[3] == 'l');
        STD_INSIST(sv[4] == 'o');
    }

    STD_TEST(BackMethod) {
        StringView sv("test");
        STD_INSIST(sv.back() == 't');
    }

    STD_TEST(BackMethodSingleChar) {
        StringView sv("x");
        STD_INSIST(sv.back() == 'x');
    }

    STD_TEST(IteratorBeginEnd) {
        StringView sv("abc");
        auto it = sv.begin();
        STD_INSIST(*it == 'a');
        ++it;
        STD_INSIST(*it == 'b');
        ++it;
        STD_INSIST(*it == 'c');
        ++it;
        STD_INSIST(it == sv.end());
    }

    STD_TEST(IteratorEmptyString) {
        StringView sv("");
        STD_INSIST(sv.begin() == sv.end());
    }

    STD_TEST(MutBeginEnd) {
        const u8* data = (const u8*)"test";
        StringView sv(data, 4);
        auto it = sv.mutBegin();
        STD_INSIST(it == sv.data());
        STD_INSIST(sv.mutEnd() == sv.mutBegin() + 4);
    }

    STD_TEST(EqualityOperator) {
        StringView sv1("hello");
        StringView sv2("hello");
        STD_INSIST(sv1 == sv2);
    }

    STD_TEST(EqualityDifferentStrings) {
        StringView sv1("hello");
        StringView sv2("world");
        STD_INSIST(!(sv1 == sv2));
    }

    STD_TEST(EqualityDifferentLengths) {
        StringView sv1("hello");
        StringView sv2("hi");
        STD_INSIST(!(sv1 == sv2));
    }

    STD_TEST(EqualityEmptyStrings) {
        StringView sv1("");
        StringView sv2("");
        STD_INSIST(sv1 == sv2);
    }

    STD_TEST(InequalityOperator) {
        StringView sv1("hello");
        StringView sv2("world");
        STD_INSIST(sv1 != sv2);
    }

    STD_TEST(InequalityEqualStrings) {
        StringView sv1("test");
        StringView sv2("test");
        STD_INSIST(!(sv1 != sv2));
    }

    STD_TEST(LessThanOperator) {
        StringView sv1("abc");
        StringView sv2("def");
        STD_INSIST(sv1 < sv2);
    }

    STD_TEST(LessThanSamePrefix) {
        StringView sv1("test");
        StringView sv2("testing");
        STD_INSIST(sv1 < sv2);
    }

    STD_TEST(LessThanEqual) {
        StringView sv1("test");
        StringView sv2("test");
        STD_INSIST(!(sv1 < sv2));
    }

    STD_TEST(SpaceshipLess) {
        StringView sv1("aaa");
        StringView sv2("bbb");
        STD_INSIST(spaceship(sv1, sv2) < 0);
    }

    STD_TEST(SpaceshipEqual) {
        StringView sv1("test");
        StringView sv2("test");
        STD_INSIST(spaceship(sv1, sv2) == 0);
    }

    STD_TEST(SpaceshipGreater) {
        StringView sv1("zzz");
        StringView sv2("aaa");
        STD_INSIST(spaceship(sv1, sv2) > 0);
    }

    STD_TEST(SpaceshipDifferentLengths) {
        StringView sv1("test");
        StringView sv2("te");
        STD_INSIST(spaceship(sv1, sv2) > 0);
    }

    STD_TEST(Hash32) {
        StringView sv("test");
        u32 h = sv.hash32();
        STD_INSIST(h != 0);
    }

    STD_TEST(Hash32Consistency) {
        StringView sv1("test");
        StringView sv2("test");
        STD_INSIST(sv1.hash32() == sv2.hash32());
    }

    STD_TEST(Hash32Different) {
        StringView sv1("test");
        StringView sv2("best");
        STD_INSIST(sv1.hash32() != sv2.hash32());
    }

    STD_TEST(Hash64) {
        StringView sv("test");
        u64 h = sv.hash64();
        STD_INSIST(h != 0);
    }

    STD_TEST(Hash64Consistency) {
        StringView sv1("test");
        StringView sv2("test");
        STD_INSIST(sv1.hash64() == sv2.hash64());
    }

    STD_TEST(Hash64Different) {
        StringView sv1("test");
        StringView sv2("best");
        STD_INSIST(sv1.hash64() != sv2.hash64());
    }

    STD_TEST(SingleCharacterString) {
        StringView sv("x");
        STD_INSIST(sv.length() == 1);
        STD_INSIST(sv[0] == 'x');
        STD_INSIST(!sv.empty());
    }

    STD_TEST(LongString) {
        const char* long_str = "This is a very long string that tests the StringView class with more content";
        StringView sv(long_str);
        STD_INSIST(sv.length() == 76);
        STD_INSIST(sv[0] == 'T');
        STD_INSIST(sv.back() == 't');
    }

    STD_TEST(SpecialCharacters) {
        StringView sv("tab\there\nnewline");
        STD_INSIST(sv.length() == 16);
        STD_INSIST(sv[3] == '\t');
        STD_INSIST(sv[8] == '\n');
    }

    STD_TEST(NullByteInMiddle) {
        const u8 data[] = {u8't', u8'e', u8's', u8't', 0, u8'e', u8'n', u8'd'};
        StringView sv(data, 8);
        STD_INSIST(sv.length() == 8);
        STD_INSIST(sv[4] == 0);
    }

    STD_TEST(CompareWithDifferentTypes) {
        StringView sv("test");
        const u8 arr[] = u8"test";
        StringView sv2(arr);
        STD_INSIST(sv == sv2);
    }

    STD_TEST(IteratorLoop) {
        StringView sv("abc");
        u8 expected[] = {u8'a', u8'b', u8'c'};
        size_t i = 0;
        for (auto it = sv.begin(); it != sv.end(); ++it) {
            STD_INSIST(*it == expected[i]);
            ++i;
        }
        STD_INSIST(i == 3);
    }

    STD_TEST(EmptyHash) {
        StringView sv("");
        u32 h32 = sv.hash32();
        u64 h64 = sv.hash64();
        STD_INSIST(h32 == 0 || h32 != 0);
        STD_INSIST(h64 == 0 || h64 != 0);
    }

    STD_TEST(ConstructFromDynString) {
        DynString ds("dynamic");
        StringView sv(ds);
        STD_INSIST(sv.length() == 7);
        STD_INSIST(sv[0] == 'd');
    }

    STD_TEST(EqualityWithDynString) {
        DynString ds("test");
        StringView sv("test");
        STD_INSIST(sv == StringView(ds));
    }

    STD_TEST(MultipleViewsSameData) {
        const u8* data = (const u8*)"shared";
        StringView sv1(data, 6);
        StringView sv2(data, 6);
        STD_INSIST(sv1.data() == sv2.data());
        STD_INSIST(sv1 == sv2);
    }

    STD_TEST(PartialView) {
        const u8* data = (const u8*)"hello world";
        StringView sv1(data, 5);
        StringView sv2(data + 6, 5);
        STD_INSIST(sv1.length() == 5);
        STD_INSIST(sv2.length() == 5);
        STD_INSIST(sv1 != sv2);
    }

    STD_TEST(Assignment) {
        StringView sv1("first");
        StringView sv2("second");
        sv1 = sv2;
        STD_INSIST(sv1 == sv2);
        STD_INSIST(sv1.length() == 6);
    }

    STD_TEST(SelfAssignment) {
        StringView sv("test");
        sv = sv;
        STD_INSIST(sv.length() == 4);
    }

    STD_TEST(CompareEmptyWithNonEmpty) {
        StringView sv1("");
        StringView sv2("a");
        STD_INSIST(sv1 < sv2);
        STD_INSIST(spaceship(sv1, sv2) < 0);
    }

    STD_TEST(NumericCharacters) {
        StringView sv("0123456789");
        STD_INSIST(sv.length() == 10);
        STD_INSIST(sv[0] == '0');
        STD_INSIST(sv[9] == '9');
    }

    STD_TEST(UnicodeBytes) {
        const u8 utf8_data[] = {0xD0, 0xBF, 0xD1, 0x80, 0xD0, 0xB8, 0xD0, 0xB2, 0xD0, 0xB5, 0xD1, 0x82, 0};
        StringView sv(utf8_data, 12);
        STD_INSIST(sv.length() == 12);
    }
}
