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

        static auto dark(AnsiColor c) {
            return Color{c, false};
        }

        static auto bright(AnsiColor c) {
            return Color{c, true};
        }

        static auto reset() {
            return Color{AnsiColor::Reset, false};
        }
    };
}
