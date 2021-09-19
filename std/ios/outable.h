#pragma once

#include <std/typ/traits.h>

namespace Std {
    template <typename O, typename T>
    void output(O& out, Traits::FuncParam<T> t);
}
