#pragma once

#include "disposable.h"

#include <std/lib/list.h>

namespace Std {
    class Disposer {
        IntrusiveList lst;

    public:
        inline ~Disposer() noexcept {
            dispose();
        }

        void dispose() noexcept;

        inline void submit(Disposable* d) noexcept {
            lst.pushBack(d);
        }

        inline unsigned length() const noexcept {
            return lst.length();
        }
    };
}
