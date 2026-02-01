#pragma once

namespace Std {
    struct Disposable {
        Disposable* prev = 0;

        virtual ~Disposable() noexcept;
    };
}
