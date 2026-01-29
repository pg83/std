#pragma once

#include <std/typ/intrin.h>

namespace Std {
    template <typename T>
    concept PassByVal = sizeof(T) <= sizeof(void*) && stdIsTriviallyCopyable(T);

    template <typename T>
    concept PassByRef = !PassByVal<T>;

    template <typename O, PassByVal T>
    void output(O& out, T t);

    template <typename O, PassByRef T>
    void output(O& out, const T& t);
}
