#pragma once

#include "param.h"

#include <std/lib/support.h>

namespace Std {
    template <typename O, typename T>
    void output(O& out, Meta::FuncParam<T> t);

    struct OutAble {
        template <typename O, typename T>
        friend inline O& operator<<(O& out, const T& t) {
            output<O, T>(out, t);

            return out;
        }

        template <typename O, typename T>
        friend inline O&& operator<<(O&& out, const T& t) {
            output<O, T>(out, t);

            return move(out);
        }
    };
}
