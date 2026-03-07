#pragma once

#include "disposable.h"

namespace stl {
    class Disposer {
        Disposable* end = 0;

    public:
        ~Disposer() noexcept {
            dispose();
        }

        void dispose() noexcept;

        void submit(Disposable* d) noexcept {
            d->prev = end;
            end = d;
        }

        unsigned length() const noexcept;
    };
}
