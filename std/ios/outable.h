#pragma once

#include "param.h"

#include <std/lib/support.h>

namespace Std {
    template <typename O, typename T>
    void output(O& out, Meta::FuncParam<T> t);

    template <typename B, typename D>
    struct IsBaseOf {
        enum {
            R = __is_base_of(B, D)
        };
    };

    template <typename B, typename D, typename T>
    using EnableForDerived = Meta::EnableIf<IsBaseOf<B, RemoveReference<D>>::R, T>;
}
