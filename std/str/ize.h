#pragma once

#include "char.h"
#include "view.h"

namespace Std {
    inline auto stringize(u8 ch) noexcept {
        return OneCharString(ch);
    }

    inline auto stringize(char ch) noexcept {
        return OneCharString(ch);
    }

    template <typename S>
    inline auto stringize(const S& str) noexcept {
        return StringView(str);
    }
}
