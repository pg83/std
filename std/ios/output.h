#pragma once

#include <std/sys/types.h>

namespace Std {
    class StringView;

    struct Output {
        virtual ~Output() noexcept;

        inline void write(const void* data, size_t len) {
            writeImpl(data, len);
        }

        inline void writeV(const StringView* parts, size_t count) {
            writeVImpl(parts, count);
        }

        inline void flush() {
            flushImpl();
        }

        inline void finish() {
            finishImpl();
        }

    private:
        virtual void writeImpl(const void* data, size_t len) = 0;

        // have sensible defaults
        virtual void writeVImpl(const StringView* parts, size_t count);
        virtual void flushImpl();
        virtual void finishImpl();
    };
}
