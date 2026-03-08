#pragma once

#include <std/ios/out_zc.h>
#include <std/lib/buffer.h>

namespace stl {
    class StringBuilder: public ZeroCopyOutput, public Buffer {
        size_t writeImpl(const void* ptr, size_t len) override;
        void* imbueImpl(size_t* len) override;
        void commitImpl(size_t len) override;

    public:
        StringBuilder();
        StringBuilder(size_t reserve);
        StringBuilder(Buffer&& buf);

        ~StringBuilder() override;
    };
}
