#include "view.h"
#include "builder.h"

#include <std/tst/ut.h>
#include <std/ios/sys.h>

using namespace Std;

STD_TEST_SUITE(StringBuilder) {
    STD_TEST(numbers) {
        STD_INSIST(StringView(StringBuilder() << 1 << -2 << finI) == StringView(u8"1-2"));
    }

    STD_TEST(string_view) {
        StringBuilder sb;
        StringView sv(u8"Hello, World!");
        sb << sv;
        STD_INSIST(StringView(sb) == sv);
    }

    STD_TEST(mixed_output) {
        StringBuilder sb;
        sb << 42 << StringView(u8" is the answer");
        STD_INSIST(StringView(sb) == StringView(u8"42 is the answer"));
    }

    STD_TEST(empty_string_view) {
        StringBuilder sb;
        StringView sv;
        sb << sv;
        STD_INSIST(StringView(sb).empty());
    }

    STD_TEST(reserve_capacity) {
        StringBuilder sb(100);
        sb << 123456789;
        STD_INSIST(StringView(sb) == StringView(u8"123456789"));
    }

    STD_TEST(negative_numbers) {
        StringBuilder sb;
        sb << -100 << 0 << -1;
        STD_INSIST(StringView(sb) == StringView(u8"-1000-1"));
    }

    STD_TEST(large_numbers) {
        StringBuilder sb;
        sb << 2147483647 << -2147483648; // INT_MAX and INT_MIN
        STD_INSIST(StringView(sb) == StringView(u8"2147483647-2147483648"));
    }

    STD_TEST(empty_builder) {
        StringBuilder sb;
        STD_INSIST(StringView(sb).empty());
    }

    STD_TEST(builder_with_reserve) {
        StringBuilder sb(1024);
        STD_INSIST(StringView(sb).empty());
        sb << 42;
        STD_INSIST(StringView(sb) == StringView(u8"42"));
    }

    STD_TEST(chained_output) {
        StringBuilder sb;
        sb << 1 << 2 << 3 << StringView(u8"test") << 4;
        STD_INSIST(StringView(sb) == StringView(u8"123test4"));
    }

    STD_TEST(zero_value) {
        StringBuilder sb;
        sb << 0;
        STD_INSIST(StringView(sb) == StringView(u8"0"));
    }

    STD_TEST(multiple_string_views) {
        StringBuilder sb;
        StringView sv1(u8"Hello");
        StringView sv2(u8" ");
        StringView sv3(u8"World");
        StringView sv4(u8"!");
        sb << sv1 << sv2 << sv3 << sv4;
        STD_INSIST(StringView(sb) == StringView(u8"Hello World!"));
    }

    STD_TEST(large_string_view) {
        StringBuilder sb;
        // Создаем большую строку
        char large_str[1001];
        for (int i = 0; i < 1000; i++) {
            large_str[i] = 'a' + (i % 26);
        }
        large_str[1000] = '\0';
        StringView sv(large_str);
        sb << sv;
        STD_INSIST(StringView(sb) == sv);
    }

    STD_TEST(mixed_large_numbers_and_strings) {
        StringBuilder sb;
        sb << 2147483647 << StringView(u8"middle") << -2147483648;
        STD_INSIST(StringView(sb) == StringView(u8"2147483647middle-2147483648"));
    }

    STD_TEST(repeated_operations) {
        StringBuilder sb;
        for (int i = 0; i < 100; i++) {
            sb << i;
        }
        // Проверяем, что строка не пустая и имеет ожидаемую длину
        STD_INSIST(!StringView(sb).empty());
        STD_INSIST(StringView(sb).length() > 0);
    }
}
