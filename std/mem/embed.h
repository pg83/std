#pragma once

#include <std/typ/support.h>

namespace Std {
    template <typename T>
    struct Embed {
        T t;

        template <typename... A>
        inline Embed(A&&... a)
            : t(forward<A>(a)...)
        {
        }
    };
}
