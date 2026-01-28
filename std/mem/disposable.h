#pragma once

#include <std/lib/node.h>

namespace Std {
    struct Disposable: public IntrusiveNode {
        virtual ~Disposable() noexcept;
    };
}
