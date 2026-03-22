#pragma once

#include "in_zc.h"

namespace stl {
    class ZeroInput: public ZeroCopyInput {
        size_t nextImpl(const void** chunk) override;
        void commitImpl(size_t len) noexcept override;
    };
}
