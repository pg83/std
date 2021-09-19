#pragma once

#include <std/str/view.h>

namespace Std {
    enum class AnsiColor {
        Reset = 0,

        Black,
        Red,
        Green,
        Yellow,
        Blue,
        Magenta,
        Cyan,
        White,
    };

    struct Color {
        AnsiColor color;
        bool brightKind;

        static inline auto dark(AnsiColor c) noexcept {
            return Color{c, false};
        }

        static inline auto bright(AnsiColor c) noexcept {
            return Color{c, true};
        }

        static inline auto reset() noexcept {
            return Color{AnsiColor::Reset, false};
        }
    };
}
