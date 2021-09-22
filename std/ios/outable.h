#pragma once

#include <std/typ/meta.h>
#include <std/typ/intrin.h>

namespace Std::IOSP {
    // how to pass function params
    template <typename T>
    struct PassByValue: public Meta::Bool<sizeof(T) <= sizeof(void*) && stdIsTriviallyCopyable(T)> {
    };

    template <typename T>
    using FuncParam = Meta::Select<PassByValue<T>::R, T, const T&>;
}

namespace Std {
    template <typename O, typename T>
    void output(O& out, IOSP::FuncParam<T> t);
}
