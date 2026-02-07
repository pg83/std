#pragma once

#include <std/sys/types.h>

struct iovec;

namespace Std {
    class StringView;

    class Output {
        virtual size_t writeImpl(const void* data, size_t len) = 0;
        virtual size_t hintImpl() const noexcept;

        // have sensible defaults
        virtual size_t writeVImpl(iovec* parts, size_t count);

        virtual void flushImpl();
        virtual void finishImpl();

    public:
        virtual ~Output() noexcept;

        void writeH(const void* data, size_t len);
        void writeC(const void* data, size_t len);

        size_t writeP(const void* data, size_t len);

        size_t writeV(iovec* parts, size_t count);
        size_t writeV(const StringView* parts, size_t count);

        bool hint(size_t* res) const noexcept;

        inline size_t write(const void* data, size_t len) {
            return (writeC(data, len), len);
        }

        inline void flush() {
            flushImpl();
        }

        inline void finish() {
            finishImpl();
        }
    };
}
