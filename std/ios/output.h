#pragma once

#include <std/sys/types.h>

namespace Std {
    class StringView;

    struct Output {
        virtual ~Output() noexcept;

        void write(const void* data, size_t len);

        inline void writeV(const StringView* parts, size_t count) {
            writeVImpl(parts, count);
        }

        inline void flush() {
            flushImpl();
        }

        inline void finish() {
            finishImpl();
        }

        inline size_t hint() const noexcept {
            return hintImpl();
        }

    private:
        virtual void writeImpl(const void* data, size_t len) = 0;
        virtual size_t hintImpl() const noexcept = 0;

        // have sensible defaults
        virtual void writeVImpl(const StringView* parts, size_t count);
        virtual void flushImpl();
        virtual void finishImpl();
    };
}
