#pragma once

#include <std/sys/types.h>

struct iovec;

namespace Std {
    class StringView;

    class Output {
        virtual void writeImpl(const void* data, size_t len) = 0;
        virtual size_t hintImpl() const noexcept = 0;

        // have sensible defaults
        virtual void writeVImpl(const iovec* parts, size_t count);
        virtual void flushImpl();
        virtual void finishImpl();

    public:
        virtual ~Output() noexcept;

        void write(const void* data, size_t len);

        inline void writeV(const iovec* parts, size_t count) {
            writeVImpl(parts, count);
        }

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
