#include "dynamic.h"

#include <std/tst/ut.h>

using namespace Std;

STD_TEST_SUITE(DynString) {
    STD_TEST(DefaultConstruction) {
        DynString s;
        STD_INSIST(s.empty());
        STD_INSIST(s.length() == 0);
    }

    STD_TEST(ConstructionFromCString) {
        DynString s("hello");
        STD_INSIST(!s.empty());
        STD_INSIST(s.length() == 5);
    }

    STD_TEST(ConstructionFromString) {
        DynString s1("test");
        DynString s2(s1);
        STD_INSIST(s2.length() == 4);
    }

    STD_TEST(MoveConstruction) {
        DynString s1("move");
        DynString s2(move(s1));
        STD_INSIST(s2.length() == 4);
        STD_INSIST(s1.empty());
    }

    STD_TEST(CopyConstruction) {
        DynString s1("copy");
        DynString s2(s1);
        STD_INSIST(s1.length() == 4);
        STD_INSIST(s2.length() == 4);
    }

    STD_TEST(CStr) {
        DynString s("test");
        char* cstr = s.cStr();
        STD_INSIST(cstr[0] == 't');
        STD_INSIST(cstr[1] == 'e');
        STD_INSIST(cstr[2] == 's');
        STD_INSIST(cstr[3] == 't');
        STD_INSIST(cstr[4] == '\0');
    }

    STD_TEST(CStrEmpty) {
        DynString s;
        char* cstr = s.cStr();
        STD_INSIST(cstr[0] == '\0');
    }

    STD_TEST(AppendMethod) {
        DynString s;
        const u8* data = (const u8*)"hello";
        s.append(data, 5);
        STD_INSIST(s.length() == 5);
    }

    STD_TEST(AppendToExisting) {
        DynString s("hello");
        const u8* data = (const u8*)" world";
        s.append(data, 6);
        STD_INSIST(s.length() == 11);
    }

    STD_TEST(PlusEqualOperator) {
        DynString s("hello");
        s += " world";
        STD_INSIST(s.length() == 11);
    }

    STD_TEST(PlusEqualEmpty) {
        DynString s;
        s += "test";
        STD_INSIST(s.length() == 4);
    }

    STD_TEST(PlusEqualMultiple) {
        DynString s;
        s += "a";
        s += "b";
        s += "c";
        STD_INSIST(s.length() == 3);
    }

    STD_TEST(PlusOperator) {
        DynString s1("hello");
        DynString s2 = s1 + " world";
        STD_INSIST(s2.length() == 11);
        STD_INSIST(s1.length() == 5);
    }

    STD_TEST(PlusOperatorChained) {
        DynString s = DynString("a") + "b" + "c";
        STD_INSIST(s.length() == 3);
    }

    STD_TEST(PlusOperatorEmpty) {
        DynString s1;
        DynString s2 = s1 + "test";
        STD_INSIST(s2.length() == 4);
        STD_INSIST(s1.empty());
    }

    STD_TEST(AppendZeroLength) {
        DynString s("test");
        const u8* data = (const u8*)"";
        s.append(data, 0);
        STD_INSIST(s.length() == 4);
    }

    STD_TEST(MultipleAppends) {
        DynString s;
        const u8* d1 = (const u8*)"hello";
        const u8* d2 = (const u8*)" ";
        const u8* d3 = (const u8*)"world";
        s.append(d1, 5);
        s.append(d2, 1);
        s.append(d3, 5);
        STD_INSIST(s.length() == 11);
    }

    STD_TEST(ClearAndReuse) {
        DynString s("test");
        s.clear();
        STD_INSIST(s.empty());
        s += "new";
        STD_INSIST(s.length() == 3);
    }

    STD_TEST(LongString) {
        DynString s;
        for (u32 i = 0; i < 100; ++i) {
            s += "x";
        }
        STD_INSIST(s.length() == 100);
    }

    STD_TEST(PlusEqualDynString) {
        DynString s1("hello");
        DynString s2(" world");
        s1 += s2;
        STD_INSIST(s1.length() == 11);
    }

    STD_TEST(PlusOperatorDynString) {
        DynString s1("hello");
        DynString s2(" world");
        DynString s3 = s1 + s2;
        STD_INSIST(s3.length() == 11);
        STD_INSIST(s1.length() == 5);
        STD_INSIST(s2.length() == 6);
    }

    STD_TEST(MovePreservesContent) {
        DynString s1("content");
        DynString s2(move(s1));
        char* cstr = s2.cStr();
        STD_INSIST(cstr[0] == 'c');
        STD_INSIST(cstr[6] == 't');
    }

    STD_TEST(AppendReturnsSelf) {
        DynString s;
        const u8* d1 = (const u8*)"a";
        const u8* d2 = (const u8*)"b";
        s.append(d1, 1).append(d2, 1);
        STD_INSIST(s.length() == 2);
    }

    STD_TEST(PlusEqualReturnsSelf) {
        DynString s;
        (s += "a") += "b";
        STD_INSIST(s.length() == 2);
    }

    STD_TEST(EmptyStringOperations) {
        DynString s1;
        DynString s2;
        DynString s3 = s1 + s2;
        STD_INSIST(s3.empty());
    }

    STD_TEST(SingleCharacter) {
        DynString s("a");
        STD_INSIST(s.length() == 1);
        char* cstr = s.cStr();
        STD_INSIST(cstr[0] == 'a');
        STD_INSIST(cstr[1] == '\0');
    }

    STD_TEST(SpecialCharacters) {
        DynString s("tab\there\nnewline");
        STD_INSIST(s.length() > 0);
        char* cstr = s.cStr();
        STD_INSIST(cstr[3] == '\t');
        STD_INSIST(cstr[8] == '\n');
    }

    STD_TEST(ComplexChaining) {
        DynString s;
        s += "a";
        s = s + "b";
        s += "c";
        s = s + "d";
        STD_INSIST(s.length() == 4);
    }

    STD_TEST(CopyDoesNotAffectOriginal) {
        DynString s1("original");
        DynString s2(s1);
        s2 += " modified";
        STD_INSIST(s1.length() == 8);
        STD_INSIST(s2.length() == 17);
    }
}
