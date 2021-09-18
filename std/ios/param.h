#pragma once

#include <std/lib/meta.h>

namespace Std::Meta {
    template <typename T>
    struct PassByValue {
        enum {
            R = sizeof(T) <= sizeof(void*) && __is_trivially_copyable(T)
        };
    };

    template <typename T>
    using FuncParam = Select<PassByValue<T>::R, T, const T&>;
}
