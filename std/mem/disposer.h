#pragma once

#include "disposable.h"

namespace stl {
    class Disposer {
        Disposable* end = 0;

    public:
        ~Disposer() {
            dispose();
        }

        void dispose();

        void submit(Disposable* d) {
            d->prev = end;
            end = d;
        }

        unsigned length() const;
    };
}
