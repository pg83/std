#pragma once

namespace stl {
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

        static auto dark(AnsiColor c) noexcept {
            return Color{c, false};
        }

        static auto bright(AnsiColor c) noexcept {
            return Color{c, true};
        }

        static auto reset() noexcept {
            return Color{AnsiColor::Reset, false};
        }
    };
}
