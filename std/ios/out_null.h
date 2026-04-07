#pragma once

#include "output.h"

namespace stl {
    class NullOutput: public Output {
        size_t writeImpl(const void* data, size_t len) override;

    public:
        ~NullOutput() noexcept override;
    };
}
