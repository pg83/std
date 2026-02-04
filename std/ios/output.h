#pragma once

#include <std/sys/types.h>

struct iovec;

namespace Std {
    class StringView;

    class Output {
        virtual size_t writeImpl(const void* data, size_t len) = 0;
        virtual size_t hintImpl() const noexcept = 0;

        // have sensible defaults
        virtual size_t writeVImpl(iovec* parts, size_t count);

        virtual void flushImpl();
        virtual void finishImpl();

    public:
        virtual ~Output() noexcept;

        void write(const void* data, size_t len);
        void writeV(iovec* parts, size_t count);
        void writeV(const StringView* parts, size_t count);

        inline void flush() {
            flushImpl();
        }

        inline void finish() {
            finishImpl();
        }

        inline size_t hint() const noexcept {
            return hintImpl();
        }
    };
}
