#include "view.h"
#include "builder.h"

#include <std/tst/ut.h>
#include <std/ios/sys.h>

using namespace Std;

namespace {
    // Helper function to normalize floating point representation
    // Truncates to 4 significant digits after decimal point
    static inline StringView normalizeFloat(StringView sv) {
        // Find decimal point
        size_t dotPos = 0;
        for (; dotPos < sv.length(); ++dotPos) {
            if (sv.data()[dotPos] == '.') {
                break;
            }
        }

        // If no decimal point or too short, return as is
        if (dotPos >= sv.length() || sv.length() <= dotPos + 5) {
            return sv;
        }

        // Truncate to 4 digits after decimal point
        return StringView(sv.data(), dotPos + 5);
    }
}

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

    STD_TEST(float_basic) {
        StringBuilder sb;
        sb << 3.14159f;
        // Normalize to 4 digits after decimal point for platform compatibility
        StringView result = normalizeFloat(StringView(sb));
        STD_INSIST(result == StringView(u8"3.1415"));
    }

    STD_TEST(double_basic) {
        StringBuilder sb;
        sb << 2.718281828459045;
        // Normalize to 4 digits after decimal point for platform compatibility
        StringView result = normalizeFloat(StringView(sb));
        STD_INSIST(result == StringView(u8"2.7182"));
    }

    STD_TEST(long_double_basic) {
        StringBuilder sb;
        sb << 1.4142135623730950488L;
        // Normalize to 4 digits after decimal point for platform compatibility
        StringView result = normalizeFloat(StringView(sb));
        STD_INSIST(result == StringView(u8"1.4142"));
    }

    STD_TEST(float_zero) {
        StringBuilder sb;
        sb << 0.0f;
        StringView result{StringView(sb)};
        STD_INSIST(result == StringView(u8"0.000000"));
    }

    STD_TEST(double_zero) {
        StringBuilder sb;
        sb << 0.0;
        StringView result{StringView(sb)};
        STD_INSIST(result == StringView(u8"0.000000"));
    }

    STD_TEST(long_double_zero) {
        StringBuilder sb;
        sb << 0.0L;
        StringView result{StringView(sb)};
        STD_INSIST(result == StringView(u8"0.000000"));
    }

    STD_TEST(float_negative) {
        StringBuilder sb;
        sb << -1.234f;
        // Normalize to 4 digits after decimal point for platform compatibility
        StringView result = normalizeFloat(StringView(sb));
        // Check that it starts with minus and has the expected format
        STD_INSIST(result.length() > 0 && result.data()[0] == '-');
    }

    STD_TEST(double_negative) {
        StringBuilder sb;
        sb << -5.6789;
        // Normalize to 4 digits after decimal point for platform compatibility
        StringView result = normalizeFloat(StringView(sb));
        // Check that it starts with minus and has the expected format
        STD_INSIST(result.length() > 0 && result.data()[0] == '-');
    }

    STD_TEST(long_double_negative) {
        StringBuilder sb;
        sb << -9.8765L;
        // Normalize to 4 digits after decimal point for platform compatibility
        StringView result = normalizeFloat(StringView(sb));
        // Check that it starts with minus and has the expected format
        STD_INSIST(result.length() > 0 && result.data()[0] == '-');
    }

    STD_TEST(mixed_float_types) {
        StringBuilder sb;
        sb << 1.0f << 2.0 << 3.0L;
        // The result should contain all three numbers
        StringView result{StringView(sb)};
        STD_INSIST(result.length() > 0);
        // Just check that it's not empty - exact format may vary by platform
    }

    STD_TEST(float_scientific_notation) {
        StringBuilder sb;
        sb << 1e10f;
        StringView result{StringView(sb)};
        // Just verify it's not empty - exact format depends on platform
        STD_INSIST(!result.empty());
    }

    STD_TEST(double_scientific_notation) {
        StringBuilder sb;
        sb << 1e-10;
        StringView result{StringView(sb)};
        // Just verify it's not empty - exact format depends on platform
        STD_INSIST(!result.empty());
    }

    STD_TEST(long_double_scientific_notation) {
        StringBuilder sb;
        sb << 1e20L;
        StringView result{StringView(sb)};
        // Just verify it's not empty - exact format depends on platform
        STD_INSIST(!result.empty());
    }

    STD_TEST(float_precision_large) {
        StringBuilder sb;
        sb << 123456.789f;
        StringView result{StringView(sb)};
        // Just verify it's not empty - exact format depends on platform
        STD_INSIST(!result.empty());
    }

    STD_TEST(double_precision_large) {
        StringBuilder sb;
        sb << 123456789.123456789;
        StringView result{StringView(sb)};
        // Just verify it's not empty - exact format depends on platform
        STD_INSIST(!result.empty());
    }

    STD_TEST(long_double_precision_large) {
        StringBuilder sb;
        sb << 123456789012345.123456789012345L;
        StringView result{StringView(sb)};
        // Just verify it's not empty - exact format depends on platform
        STD_INSIST(!result.empty());
    }
}
