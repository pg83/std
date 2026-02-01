#pragma once

#include "disposable.h"

namespace Std {
    class Disposer {
        Disposable* end = 0;

    public:
        inline ~Disposer() noexcept {
            dispose();
        }

        void dispose() noexcept;

        inline void submit(Disposable* d) noexcept {
            d->prev = end;
            end = d;
        }

        unsigned length() const noexcept;
    };
}
