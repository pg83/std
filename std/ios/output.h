#pragma once

#include "unbound.h"
#include "outable.h"

#include <std/sys/types.h>
#include <std/typ/support.h>

namespace Std {
    class StringView;

    struct Output {
        virtual ~Output();

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

    struct ZeroCopyOutput: public Output {
        ~ZeroCopyOutput() override;

        // zero-copy interface
        inline UnboundBuffer imbue(size_t len) {
            return {imbueImpl(len)};
        }

        inline void bump(const void* ptr) noexcept {
            bumpImpl(ptr);
        }

        inline void bump(const UnboundBuffer& buf) noexcept {
            bump(buf.ptr);
        }

    private:
        void writeImpl(const void* data, size_t len) override;

        virtual void* imbueImpl(size_t len) = 0;
        virtual void bumpImpl(const void* ptr) noexcept = 0;
    };

    template <typename O, typename T>
    requires requires (O o) {
        static_cast<ZeroCopyOutput*>(&o);
    }
    inline O&& operator<<(O&& out, const T& t)  {
        output<ZeroCopyOutput, T>(out, t);

        return forward<O>(out);
    }
}
