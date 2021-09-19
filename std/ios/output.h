#pragma once

#include "param.h"
#include "unbound.h"
#include "outable.h"

#include <std/sys/types.h>

namespace Std {
    struct Output {
        virtual ~Output();

        inline void write(const void* data, size_t len) {
            if (len) {
                writeImpl(data, len);
            }
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
        virtual void flushImpl();
        virtual void finishImpl();
    };

    struct ZeroCopyOutput: public Output {
        virtual ~ZeroCopyOutput();

        // zero-copy interface
        inline size_t imbue(void** ptr) noexcept {
            return imbueImpl(ptr);
        }

        inline UnboundBuffer imbue(size_t len) {
            return imbueImpl(len);
        }

        inline void bump(const void* ptr) noexcept {
            return bumpImpl(ptr);
        }

        inline void bump(const UnboundBuffer& buf) noexcept {
            bump(buf.ptr);
        }

    private:
        void writeImpl(const void* data, size_t len) override;

        virtual size_t imbueImpl(void** ptr) noexcept = 0;
        virtual void* imbueImpl(size_t len) = 0;
        virtual void bumpImpl(const void* ptr) noexcept = 0;
    };

    template <typename O, typename T>
    inline EnableForDerived<ZeroCopyOutput, O, O&&> operator<<(O&& out, const T& t) {
        output<ZeroCopyOutput, T>(out, t);

        return static_cast<O&&>(out);
    }
}
