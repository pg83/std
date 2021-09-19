#pragma once

#include "ops.h"

#include <std/sys/types.h>

namespace Std {
    class OneCharString: public StringOps<OneCharString> {
        u8 char_;

    public:
        inline OneCharString(u8 c) noexcept
            : char_(c)
        {
        }

        inline auto mutData() noexcept {
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
