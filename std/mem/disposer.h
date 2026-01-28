#pragma once

#include "disposable.h"

#include <std/lib/list.h>

namespace Std {
    class Disposer {
        IntrusiveList lst;

    public:
        ~Disposer() noexcept;

        inline void submit(Disposable* d) noexcept {
            lst.pushBack(d);
        }
    };
}
