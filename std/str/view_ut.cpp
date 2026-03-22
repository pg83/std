#include "view.h"

#include <std/tst/ut.h>
#include <std/lib/buffer.h>

using namespace stl;

namespace {
    static size_t xsearch(StringView h, StringView n) noexcept {
        if (auto res = h.search(n); res) {
            return res - h.data();
        }

        return (size_t)-1;
    }
}

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
        StringView sv(data, data);
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

    STD_TEST(ConstructFromBuffer) {
        Buffer ds("dynamic");
        StringView sv(ds);
        STD_INSIST(sv.length() == 7);
        STD_INSIST(sv[0] == 'd');
    }

    STD_TEST(EqualityWithBuffer) {
        Buffer ds("test");
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

    STD_TEST(PrefixBasic) {
        StringView sv("hello world");
        StringView prefix = sv.prefix(5);
        STD_INSIST(prefix.length() == 5);
        STD_INSIST(prefix == StringView("hello"));
    }

    STD_TEST(PrefixZero) {
        StringView sv("test");
        StringView prefix = sv.prefix(0);
        STD_INSIST(prefix.length() == 0);
        STD_INSIST(prefix.empty());
    }

    STD_TEST(PrefixFullLength) {
        StringView sv("test");
        StringView prefix = sv.prefix(4);
        STD_INSIST(prefix == sv);
    }

    STD_TEST(PrefixExceedsLength) {
        StringView sv("test");
        StringView prefix = sv.prefix(100);
        STD_INSIST(prefix == sv);
        STD_INSIST(prefix.length() == 4);
    }

    STD_TEST(PrefixEmpty) {
        StringView sv("");
        StringView prefix = sv.prefix(5);
        STD_INSIST(prefix.empty());
    }

    STD_TEST(PrefixSingleChar) {
        StringView sv("hello");
        StringView prefix = sv.prefix(1);
        STD_INSIST(prefix.length() == 1);
        STD_INSIST(prefix[0] == 'h');
    }

    STD_TEST(StartsWithTrue) {
        StringView sv("hello world");
        STD_INSIST(sv.startsWith(StringView("hello")));
    }

    STD_TEST(StartsWithFalse) {
        StringView sv("hello world");
        STD_INSIST(!sv.startsWith(StringView("world")));
    }

    STD_TEST(StartsWithEmpty) {
        StringView sv("test");
        STD_INSIST(sv.startsWith(StringView("")));
    }

    STD_TEST(StartsWithEmptyString) {
        StringView sv("");
        STD_INSIST(sv.startsWith(StringView("")));
    }

    STD_TEST(StartsWithFullMatch) {
        StringView sv("test");
        STD_INSIST(sv.startsWith(StringView("test")));
    }

    STD_TEST(StartsWithLongerPrefix) {
        StringView sv("hi");
        STD_INSIST(!sv.startsWith(StringView("hello")));
    }

    STD_TEST(StartsWithSingleChar) {
        StringView sv("hello");
        STD_INSIST(sv.startsWith(StringView("h")));
    }

    STD_TEST(StartsWithNonMatchingSingleChar) {
        StringView sv("hello");
        STD_INSIST(!sv.startsWith(StringView("w")));
    }

    STD_TEST(StartsWithPartialMatch) {
        StringView sv("testing");
        STD_INSIST(sv.startsWith(StringView("test")));
        STD_INSIST(!sv.startsWith(StringView("text")));
    }

    STD_TEST(StartsWithCaseSensitive) {
        StringView sv("Hello");
        STD_INSIST(!sv.startsWith(StringView("hello")));
    }

    STD_TEST(PrefixAndStartsWith) {
        StringView sv("hello world");
        StringView prefix = sv.prefix(5);
        STD_INSIST(sv.startsWith(prefix));
    }

    STD_TEST(SuffixBasic) {
        StringView sv("hello world");
        StringView suffix = sv.suffix(5);
        STD_INSIST(suffix.length() == 5);
        STD_INSIST(suffix == StringView("world"));
    }

    STD_TEST(SuffixZero) {
        StringView sv("test");
        StringView suffix = sv.suffix(0);
        STD_INSIST(suffix.length() == 0);
        STD_INSIST(suffix.empty());
    }

    STD_TEST(SuffixFullLength) {
        StringView sv("test");
        StringView suffix = sv.suffix(4);
        STD_INSIST(suffix == sv);
    }

    STD_TEST(SuffixExceedsLength) {
        StringView sv("test");
        StringView suffix = sv.suffix(100);
        STD_INSIST(suffix == sv);
        STD_INSIST(suffix.length() == 4);
    }

    STD_TEST(SuffixEmpty) {
        StringView sv("");
        StringView suffix = sv.suffix(5);
        STD_INSIST(suffix.empty());
    }

    STD_TEST(SuffixSingleChar) {
        StringView sv("hello");
        StringView suffix = sv.suffix(1);
        STD_INSIST(suffix.length() == 1);
        STD_INSIST(suffix[0] == 'o');
    }

    STD_TEST(EndsWithTrue) {
        StringView sv("hello world");
        STD_INSIST(sv.endsWith(StringView("world")));
    }

    STD_TEST(EndsWithFalse) {
        StringView sv("hello world");
        STD_INSIST(!sv.endsWith(StringView("hello")));
    }

    STD_TEST(EndsWithEmpty) {
        StringView sv("test");
        STD_INSIST(sv.endsWith(StringView("")));
    }

    STD_TEST(EndsWithEmptyString) {
        StringView sv("");
        STD_INSIST(sv.endsWith(StringView("")));
    }

    STD_TEST(EndsWithFullMatch) {
        StringView sv("test");
        STD_INSIST(sv.endsWith(StringView("test")));
    }

    STD_TEST(EndsWithLongerSuffix) {
        StringView sv("hi");
        STD_INSIST(!sv.endsWith(StringView("hello")));
    }

    STD_TEST(EndsWithSingleChar) {
        StringView sv("hello");
        STD_INSIST(sv.endsWith(StringView("o")));
    }

    STD_TEST(EndsWithNonMatchingSingleChar) {
        StringView sv("hello");
        STD_INSIST(!sv.endsWith(StringView("w")));
    }

    STD_TEST(EndsWithPartialMatch) {
        StringView sv("testing");
        STD_INSIST(sv.endsWith(StringView("ing")));
        STD_INSIST(!sv.endsWith(StringView("int")));
    }

    STD_TEST(EndsWithCaseSensitive) {
        StringView sv("Hello");
        STD_INSIST(!sv.endsWith(StringView("HELLO")));
    }

    STD_TEST(SuffixAndEndsWith) {
        StringView sv("hello world");
        StringView suffix = sv.suffix(5);
        STD_INSIST(sv.endsWith(suffix));
    }

    STD_TEST(SearchBasic) {
        StringView sv("hello world");
        size_t pos = xsearch(sv, StringView("world"));
        STD_INSIST(pos == 6);
    }

    STD_TEST(SearchAtBeginning) {
        StringView sv("hello world");
        size_t pos = xsearch(sv, StringView("hello"));
        STD_INSIST(pos == 0);
    }

    STD_TEST(SearchNotFound) {
        StringView sv("hello world");
        size_t pos = xsearch(sv, StringView("xyz"));
        STD_INSIST(pos == (size_t)-1);
    }

    STD_TEST(SearchEmptySubstring) {
        StringView sv("test");
        size_t pos = xsearch(sv, StringView(""));
        STD_INSIST(pos == 0);
    }

    STD_TEST(SearchInEmptyString) {
        StringView sv("");
        size_t pos = xsearch(sv, StringView("test"));
        STD_INSIST(pos == (size_t)-1);
    }

    STD_TEST(SearchEmptyInEmpty) {
        StringView sv("");
        size_t pos = xsearch(sv, StringView(""));
        STD_INSIST(pos == 0);
    }

    STD_TEST(SearchSingleChar) {
        StringView sv("hello");
        size_t pos = xsearch(sv, StringView("e"));
        STD_INSIST(pos == 1);
    }

    STD_TEST(SearchSingleCharAtEnd) {
        StringView sv("hello");
        size_t pos = xsearch(sv, StringView("o"));
        STD_INSIST(pos == 4);
    }

    STD_TEST(SearchSingleCharNotFound) {
        StringView sv("hello");
        size_t pos = xsearch(sv, StringView("x"));
        STD_INSIST(pos == (size_t)-1);
    }

    STD_TEST(SearchFullMatch) {
        StringView sv("test");
        size_t pos = xsearch(sv, StringView("test"));
        STD_INSIST(pos == 0);
    }

    STD_TEST(SearchLongerSubstring) {
        StringView sv("test");
        size_t pos = xsearch(sv, StringView("testing"));
        STD_INSIST(pos == (size_t)-1);
    }

    STD_TEST(SearchRepeatingPattern) {
        StringView sv("abababab");
        size_t pos = xsearch(sv, StringView("abab"));
        STD_INSIST(pos == 0);
    }

    STD_TEST(SearchRepeatingPatternMiddle) {
        StringView sv("xxabababxx");
        size_t pos = xsearch(sv, StringView("abab"));
        STD_INSIST(pos == 2);
    }

    STD_TEST(SearchMultipleOccurrencesFirst) {
        StringView sv("hello hello hello");
        size_t pos = xsearch(sv, StringView("hello"));
        STD_INSIST(pos == 0);
    }

    STD_TEST(SearchPartialMatchThenFullMatch) {
        StringView sv("aaab");
        size_t pos = xsearch(sv, StringView("aab"));
        STD_INSIST(pos == 1);
    }

    STD_TEST(SearchCaseSensitive) {
        StringView sv("Hello World");
        size_t pos = xsearch(sv, StringView("world"));
        STD_INSIST(pos == (size_t)-1);
    }

    STD_TEST(SearchCaseSensitiveMatch) {
        StringView sv("Hello World");
        size_t pos = xsearch(sv, StringView("World"));
        STD_INSIST(pos == 6);
    }

    STD_TEST(SearchWithSpaces) {
        StringView sv("hello world test");
        size_t pos = xsearch(sv, StringView(" world "));
        STD_INSIST(pos == 5);
    }

    STD_TEST(SearchWithSpecialChars) {
        StringView sv("tab\there\nnewline");
        size_t pos = xsearch(sv, StringView("\t"));
        STD_INSIST(pos == 3);
    }

    STD_TEST(SearchWithNewline) {
        StringView sv("line1\nline2");
        size_t pos = xsearch(sv, StringView("\n"));
        STD_INSIST(pos == 5);
    }

    STD_TEST(SearchWithNullByte) {
        const u8 data[] = {u8't', u8'e', u8's', u8't', 0, u8'e', u8'n', u8'd'};
        StringView sv(data, 8);
        const u8 pattern[] = {0, u8'e'};
        size_t pos = xsearch(sv, StringView(pattern, 2));
        STD_INSIST(pos == 4);
    }

    STD_TEST(SearchLongSubstring) {
        StringView sv("this is a very long string with some text in the middle and more text at the end");
        size_t pos = xsearch(sv, StringView("text in the middle"));
        STD_INSIST(pos == 37);
    }

    STD_TEST(SearchAtVeryEnd) {
        StringView sv("hello world!");
        size_t pos = xsearch(sv, StringView("!"));
        STD_INSIST(pos == 11);
    }

    STD_TEST(SearchAlmostMatch) {
        StringView sv("testing");
        size_t pos = xsearch(sv, StringView("text"));
        STD_INSIST(pos == (size_t)-1);
    }

    STD_TEST(SearchRepeatingChars) {
        StringView sv("aaaaaaa");
        size_t pos = xsearch(sv, StringView("aaa"));
        STD_INSIST(pos == 0);
    }

    STD_TEST(SearchRepeatingCharsSuffix) {
        StringView sv("bbbbbbb");
        size_t pos = xsearch(sv, StringView("bb"));
        STD_INSIST(pos == 0);
    }

    STD_TEST(SearchNumericString) {
        StringView sv("123456789");
        size_t pos = xsearch(sv, StringView("456"));
        STD_INSIST(pos == 3);
    }

    STD_TEST(SearchWithDifferentPatterns) {
        StringView sv("abcdefghijklmnop");
        STD_INSIST(xsearch(sv, StringView("abc")) == 0);
        STD_INSIST(xsearch(sv, StringView("def")) == 3);
        STD_INSIST(xsearch(sv, StringView("nop")) == 13);
        STD_INSIST(xsearch(sv, StringView("xyz")) == (size_t)-1);
    }

    STD_TEST(SearchOverlappingPattern) {
        StringView sv("aaaa");
        size_t pos = xsearch(sv, StringView("aa"));
        STD_INSIST(pos == 0);
    }

    STD_TEST(SearchOneCharString) {
        StringView sv("x");
        size_t pos = xsearch(sv, StringView("x"));
        STD_INSIST(pos == 0);
    }

    STD_TEST(SearchOneCharStringNotFound) {
        StringView sv("x");
        size_t pos = xsearch(sv, StringView("y"));
        STD_INSIST(pos == (size_t)-1);
    }

    STD_TEST(SearchComplexPattern) {
        StringView sv("The quick brown fox jumps over the lazy dog");
        size_t pos = xsearch(sv, StringView("fox jumps"));
        STD_INSIST(pos == 16);
    }

    STD_TEST(SearchAfterFailedPartialMatch) {
        StringView sv("aaabaaab");
        size_t pos = xsearch(sv, StringView("aaab"));
        STD_INSIST(pos == 0);
    }

    STD_TEST(SearchUTF8Bytes) {
        const u8 utf8_text[] = {0xD0, 0xBF, 0xD1, 0x80, 0xD0, 0xB8, 0xD0, 0xB2, 0xD0, 0xB5, 0xD1, 0x82, 0x20, 0xD0, 0xBC, 0xD0, 0xB8, 0xD1, 0x80};
        StringView sv(utf8_text, 19);
        const u8 utf8_pattern[] = {0xD0, 0xBC, 0xD0, 0xB8, 0xD1, 0x80};
        size_t pos = xsearch(sv, StringView(utf8_pattern, 6));
        STD_INSIST(pos == 13);
    }

    STD_TEST(SearchBinaryData) {
        const u8 data[] = {0x00, 0x01, 0x02, 0x03, 0xFF, 0xFE, 0xFD};
        StringView sv(data, 7);
        const u8 pattern[] = {0xFF, 0xFE};
        size_t pos = xsearch(sv, StringView(pattern, 2));
        STD_INSIST(pos == 4);
    }

    STD_TEST(SearchEndOfString) {
        StringView sv("hello world");
        size_t pos = xsearch(sv, StringView("ld"));
        STD_INSIST(pos == 9);
    }

    STD_TEST(SearchMiddleOfRepeating) {
        StringView sv("aaabaaabaaab");
        size_t pos = xsearch(sv, StringView("baaab"));
        STD_INSIST(pos == 3);
    }

    STD_TEST(stouBasic) {
        StringView sv("123");
        STD_INSIST(sv.stou() == 123);
    }

    STD_TEST(stouZero) {
        StringView sv("0");
        STD_INSIST(sv.stou() == 0);
    }

    STD_TEST(stouEmpty) {
        StringView sv("");
        STD_INSIST(sv.stou() == 0);
    }

    STD_TEST(stouLargeNumber) {
        StringView sv("9876543210");
        STD_INSIST(sv.stou() == 9876543210ULL);
    }

    STD_TEST(stouWithLeadingZeros) {
        StringView sv("00123");
        STD_INSIST(sv.stou() == 123);
    }

    STD_TEST(stouSingleDigit) {
        StringView sv("7");
        STD_INSIST(sv.stou() == 7);
    }

    STD_TEST(stouNineNines) {
        StringView sv("999999999");
        STD_INSIST(sv.stou() == 999999999ULL);
    }

    STD_TEST(stouWithNonDigits) {
        StringView sv("123abc456");
        STD_INSIST(sv.stou() == 123456);
    }

    STD_TEST(stouOnlyNonDigits) {
        StringView sv("abc");
        STD_INSIST(sv.stou() == 0);
    }

    STD_TEST(stouLeadingNonDigits) {
        StringView sv("abc123");
        STD_INSIST(sv.stou() == 123);
    }

    STD_TEST(stouTrailingNonDigits) {
        StringView sv("123abc");
        STD_INSIST(sv.stou() == 123);
    }

    STD_TEST(stouMixedCharacters) {
        StringView sv("1a2b3c");
        STD_INSIST(sv.stou() == 123);
    }

    STD_TEST(stouWithSpaces) {
        StringView sv("1 2 3");
        STD_INSIST(sv.stou() == 123);
    }

    STD_TEST(stouMaxU64) {
        StringView sv("18446744073709551615");
        STD_INSIST(sv.stou() == 18446744073709551615ULL);
    }

    STD_TEST(stouVeryLongNumber) {
        StringView sv("12345678901234567890");
        u64 result = sv.stou();
        STD_INSIST(result > 0);
    }

    STD_TEST(stouAllZeros) {
        StringView sv("00000");
        STD_INSIST(sv.stou() == 0);
    }

    STD_TEST(stouWithNewline) {
        StringView sv("123\n456");
        STD_INSIST(sv.stou() == 123456);
    }

    STD_TEST(stouWithTab) {
        StringView sv("123\t456");
        STD_INSIST(sv.stou() == 123456);
    }

    STD_TEST(stouWithSpecialChars) {
        StringView sv("1!2@3#4$5");
        STD_INSIST(sv.stou() == 12345);
    }

    STD_TEST(StripSpaceBasic) {
        StringView sv("  hello  ");
        STD_INSIST(sv.stripSpace() == StringView("hello"));
    }

    STD_TEST(StripSpaceLeading) {
        StringView sv("   hello");
        STD_INSIST(sv.stripSpace() == StringView("hello"));
    }

    STD_TEST(StripSpaceTrailing) {
        StringView sv("hello   ");
        STD_INSIST(sv.stripSpace() == StringView("hello"));
    }

    STD_TEST(StripSpaceNone) {
        StringView sv("hello");
        STD_INSIST(sv.stripSpace() == StringView("hello"));
    }

    STD_TEST(StripSpaceEmpty) {
        StringView sv("");
        STD_INSIST(sv.stripSpace().empty());
    }

    STD_TEST(StripSpaceOnlySpaces) {
        StringView sv("   ");
        STD_INSIST(sv.stripSpace().empty());
    }

    STD_TEST(StripSpaceSingleSpace) {
        StringView sv(" ");
        STD_INSIST(sv.stripSpace().empty());
    }

    STD_TEST(StripSpaceInnerSpaces) {
        StringView sv("  hello world  ");
        STD_INSIST(sv.stripSpace() == StringView("hello world"));
    }

    STD_TEST(StripCrBasic) {
        StringView sv("hello\r");
        STD_INSIST(sv.stripCr() == StringView("hello"));
    }

    STD_TEST(StripCrNone) {
        StringView sv("hello");
        STD_INSIST(sv.stripCr() == StringView("hello"));
    }

    STD_TEST(StripCrEmpty) {
        StringView sv("");
        STD_INSIST(sv.stripCr().empty());
    }

    STD_TEST(StripCrOnlyCr) {
        StringView sv("\r");
        STD_INSIST(sv.stripCr().empty());
    }

    STD_TEST(StripCrNotAtEnd) {
        StringView sv("hel\rlo");
        STD_INSIST(sv.stripCr() == StringView("hel\rlo"));
    }

    STD_TEST(StripCrMultipleCr) {
        StringView sv("hello\r\r");
        STD_INSIST(sv.stripCr() == StringView("hello\r"));
    }

    STD_TEST(SplitBasic) {
        StringView sv("hello:world");
        StringView before, after;
        STD_INSIST(sv.split(':', before, after));
        STD_INSIST(before == StringView("hello"));
        STD_INSIST(after == StringView("world"));
    }

    STD_TEST(SplitNotFound) {
        StringView sv("hello world");
        StringView before, after;
        STD_INSIST(!sv.split(':', before, after));
    }

    STD_TEST(SplitAtStart) {
        StringView sv(":world");
        StringView before, after;
        STD_INSIST(sv.split(':', before, after));
        STD_INSIST(before.empty());
        STD_INSIST(after == StringView("world"));
    }

    STD_TEST(SplitAtEnd) {
        StringView sv("hello:");
        StringView before, after;
        STD_INSIST(sv.split(':', before, after));
        STD_INSIST(before == StringView("hello"));
        STD_INSIST(after.empty());
    }

    STD_TEST(SplitEmpty) {
        StringView sv("");
        StringView before, after;
        STD_INSIST(!sv.split(':', before, after));
    }

    STD_TEST(SplitOnlyDelim) {
        StringView sv(":");
        StringView before, after;
        STD_INSIST(sv.split(':', before, after));
        STD_INSIST(before.empty());
        STD_INSIST(after.empty());
    }

    STD_TEST(SplitFirstOccurrence) {
        StringView sv("a:b:c");
        StringView before, after;
        STD_INSIST(sv.split(':', before, after));
        STD_INSIST(before == StringView("a"));
        STD_INSIST(after == StringView("b:c"));
    }

    STD_TEST(SplitBySpace) {
        StringView sv("GET /index.html HTTP/1.0");
        StringView method, rest;
        STD_INSIST(sv.split(' ', method, rest));
        STD_INSIST(method == StringView("GET"));
        STD_INSIST(rest == StringView("/index.html HTTP/1.0"));
    }

    STD_TEST(LowerBasic) {
        StringView sv("Hello");
        u8 buf[8];
        STD_INSIST(sv.lower(buf) == StringView("hello"));
    }

    STD_TEST(LowerAllCaps) {
        StringView sv("HELLO");
        u8 buf[8];
        STD_INSIST(sv.lower(buf) == StringView("hello"));
    }

    STD_TEST(LowerAlreadyLower) {
        StringView sv("hello");
        u8 buf[8];
        STD_INSIST(sv.lower(buf) == StringView("hello"));
    }

    STD_TEST(LowerEmpty) {
        u8 buf[1];
        STD_INSIST(StringView("").lower(buf).empty());
    }

    STD_TEST(LowerMixed) {
        StringView sv("Content-Type");
        u8 buf[16];
        STD_INSIST(sv.lower(buf) == StringView("content-type"));
    }

    STD_TEST(LowerNonAlpha) {
        StringView sv("Host: 127.0.0.1");
        u8 buf[16];
        STD_INSIST(sv.lower(buf) == StringView("host: 127.0.0.1"));
    }

    STD_TEST(LowerDoesNotModifySource) {
        StringView sv("UPPER");
        u8 buf[8];
        sv.lower(buf);
        STD_INSIST(sv == StringView("UPPER"));
    }

    STD_TEST(StohZero) {
        STD_INSIST(StringView("0").stoh() == 0);
    }

    STD_TEST(StohBasic) {
        STD_INSIST(StringView("ff").stoh() == 255);
    }

    STD_TEST(StohUpperCase) {
        STD_INSIST(StringView("FF").stoh() == 255);
    }

    STD_TEST(StohMixedCase) {
        STD_INSIST(StringView("1a2B").stoh() == 0x1a2b);
    }

    STD_TEST(StohLarge) {
        STD_INSIST(StringView("400").stoh() == 1024);
    }
}
