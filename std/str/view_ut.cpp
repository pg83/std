#include "view.h"

#include <std/tst/ut.h>
#include <std/lib/buffer.h>

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
        size_t pos = sv.search(StringView("world"));
        STD_INSIST(pos == 6);
    }

    STD_TEST(SearchAtBeginning) {
        StringView sv("hello world");
        size_t pos = sv.search(StringView("hello"));
        STD_INSIST(pos == 0);
    }

    STD_TEST(SearchNotFound) {
        StringView sv("hello world");
        size_t pos = sv.search(StringView("xyz"));
        STD_INSIST(pos == (size_t)-1);
    }

    STD_TEST(SearchEmptySubstring) {
        StringView sv("test");
        size_t pos = sv.search(StringView(""));
        STD_INSIST(pos == 0);
    }

    STD_TEST(SearchInEmptyString) {
        StringView sv("");
        size_t pos = sv.search(StringView("test"));
        STD_INSIST(pos == (size_t)-1);
    }

    STD_TEST(SearchEmptyInEmpty) {
        StringView sv("");
        size_t pos = sv.search(StringView(""));
        STD_INSIST(pos == 0);
    }

    STD_TEST(SearchSingleChar) {
        StringView sv("hello");
        size_t pos = sv.search(StringView("e"));
        STD_INSIST(pos == 1);
    }

    STD_TEST(SearchSingleCharAtEnd) {
        StringView sv("hello");
        size_t pos = sv.search(StringView("o"));
        STD_INSIST(pos == 4);
    }

    STD_TEST(SearchSingleCharNotFound) {
        StringView sv("hello");
        size_t pos = sv.search(StringView("x"));
        STD_INSIST(pos == (size_t)-1);
    }

    STD_TEST(SearchFullMatch) {
        StringView sv("test");
        size_t pos = sv.search(StringView("test"));
        STD_INSIST(pos == 0);
    }

    STD_TEST(SearchLongerSubstring) {
        StringView sv("test");
        size_t pos = sv.search(StringView("testing"));
        STD_INSIST(pos == (size_t)-1);
    }

    STD_TEST(SearchRepeatingPattern) {
        StringView sv("abababab");
        size_t pos = sv.search(StringView("abab"));
        STD_INSIST(pos == 0);
    }

    STD_TEST(SearchRepeatingPatternMiddle) {
        StringView sv("xxabababxx");
        size_t pos = sv.search(StringView("abab"));
        STD_INSIST(pos == 2);
    }

    STD_TEST(SearchMultipleOccurrencesFirst) {
        StringView sv("hello hello hello");
        size_t pos = sv.search(StringView("hello"));
        STD_INSIST(pos == 0);
    }

    STD_TEST(SearchPartialMatchThenFullMatch) {
        StringView sv("aaab");
        size_t pos = sv.search(StringView("aab"));
        STD_INSIST(pos == 1);
    }

    STD_TEST(SearchCaseSensitive) {
        StringView sv("Hello World");
        size_t pos = sv.search(StringView("world"));
        STD_INSIST(pos == (size_t)-1);
    }

    STD_TEST(SearchCaseSensitiveMatch) {
        StringView sv("Hello World");
        size_t pos = sv.search(StringView("World"));
        STD_INSIST(pos == 6);
    }

    STD_TEST(SearchWithSpaces) {
        StringView sv("hello world test");
        size_t pos = sv.search(StringView(" world "));
        STD_INSIST(pos == 5);
    }

    STD_TEST(SearchWithSpecialChars) {
        StringView sv("tab\there\nnewline");
        size_t pos = sv.search(StringView("\t"));
        STD_INSIST(pos == 3);
    }

    STD_TEST(SearchWithNewline) {
        StringView sv("line1\nline2");
        size_t pos = sv.search(StringView("\n"));
        STD_INSIST(pos == 5);
    }

    STD_TEST(SearchWithNullByte) {
        const u8 data[] = {u8't', u8'e', u8's', u8't', 0, u8'e', u8'n', u8'd'};
        StringView sv(data, 8);
        const u8 pattern[] = {0, u8'e'};
        size_t pos = sv.search(StringView(pattern, 2));
        STD_INSIST(pos == 4);
    }

    STD_TEST(SearchLongSubstring) {
        StringView sv("this is a very long string with some text in the middle and more text at the end");
        size_t pos = sv.search(StringView("text in the middle"));
        STD_INSIST(pos == 37);
    }

    STD_TEST(SearchAtVeryEnd) {
        StringView sv("hello world!");
        size_t pos = sv.search(StringView("!"));
        STD_INSIST(pos == 11);
    }

    STD_TEST(SearchAlmostMatch) {
        StringView sv("testing");
        size_t pos = sv.search(StringView("text"));
        STD_INSIST(pos == (size_t)-1);
    }

    STD_TEST(SearchRepeatingChars) {
        StringView sv("aaaaaaa");
        size_t pos = sv.search(StringView("aaa"));
        STD_INSIST(pos == 0);
    }

    STD_TEST(SearchRepeatingCharsSuffix) {
        StringView sv("bbbbbbb");
        size_t pos = sv.search(StringView("bb"));
        STD_INSIST(pos == 0);
    }

    STD_TEST(SearchNumericString) {
        StringView sv("123456789");
        size_t pos = sv.search(StringView("456"));
        STD_INSIST(pos == 3);
    }

    STD_TEST(SearchWithDifferentPatterns) {
        StringView sv("abcdefghijklmnop");
        STD_INSIST(sv.search(StringView("abc")) == 0);
        STD_INSIST(sv.search(StringView("def")) == 3);
        STD_INSIST(sv.search(StringView("nop")) == 13);
        STD_INSIST(sv.search(StringView("xyz")) == (size_t)-1);
    }

    STD_TEST(SearchOverlappingPattern) {
        StringView sv("aaaa");
        size_t pos = sv.search(StringView("aa"));
        STD_INSIST(pos == 0);
    }

    STD_TEST(SearchOneCharString) {
        StringView sv("x");
        size_t pos = sv.search(StringView("x"));
        STD_INSIST(pos == 0);
    }

    STD_TEST(SearchOneCharStringNotFound) {
        StringView sv("x");
        size_t pos = sv.search(StringView("y"));
        STD_INSIST(pos == (size_t)-1);
    }

    STD_TEST(SearchComplexPattern) {
        StringView sv("The quick brown fox jumps over the lazy dog");
        size_t pos = sv.search(StringView("fox jumps"));
        STD_INSIST(pos == 16);
    }

    STD_TEST(SearchAfterFailedPartialMatch) {
        StringView sv("aaabaaab");
        size_t pos = sv.search(StringView("aaab"));
        STD_INSIST(pos == 0);
    }

    STD_TEST(SearchUTF8Bytes) {
        const u8 utf8_text[] = {0xD0, 0xBF, 0xD1, 0x80, 0xD0, 0xB8, 0xD0, 0xB2, 0xD0, 0xB5, 0xD1, 0x82, 0x20, 0xD0, 0xBC, 0xD0, 0xB8, 0xD1, 0x80};
        StringView sv(utf8_text, 19);
        const u8 utf8_pattern[] = {0xD0, 0xBC, 0xD0, 0xB8, 0xD1, 0x80};
        size_t pos = sv.search(StringView(utf8_pattern, 6));
        STD_INSIST(pos == 13);
    }

    STD_TEST(SearchBinaryData) {
        const u8 data[] = {0x00, 0x01, 0x02, 0x03, 0xFF, 0xFE, 0xFD};
        StringView sv(data, 7);
        const u8 pattern[] = {0xFF, 0xFE};
        size_t pos = sv.search(StringView(pattern, 2));
        STD_INSIST(pos == 4);
    }

    STD_TEST(SearchEndOfString) {
        StringView sv("hello world");
        size_t pos = sv.search(StringView("ld"));
        STD_INSIST(pos == 9);
    }

    STD_TEST(SearchMiddleOfRepeating) {
        StringView sv("aaabaaabaaab");
        size_t pos = sv.search(StringView("baaab"));
        STD_INSIST(pos == 3);
    }
}
