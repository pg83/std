#pragma once

namespace stl {
    struct Disposable {
        Disposable* prev = 0;

        virtual ~Disposable() noexcept;
    };
}
