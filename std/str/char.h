#pragma once

#include "ops.h"

#include <std/sys/types.h>

namespace Std {
    class OneCharString: public StringOps<OneCharString> {
        c8 char_;

    public:
        inline OneCharString(c8 c) noexcept
            : char_(c)
        {
        }

        inline auto data() noexcept {
            return &char_;
        }

        inline auto data() const noexcept {
            return &char_;
        }

        inline size_t length() const noexcept {
            return 1;
        }
    };
}
