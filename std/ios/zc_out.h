#pragma once

#include "output.h"
#include "unbound.h"
#include "outable.h"

#include <std/sys/types.h>
#include <std/typ/support.h>

namespace Std {
    class StringView;

    class ZeroCopyOutput: public Output {
        void writeImpl(const void* data, size_t len) override;

        virtual void* imbueImpl(size_t len) = 0;
        virtual void bumpImpl(const void* ptr) noexcept = 0;

    public:
        ~ZeroCopyOutput() noexcept override;

        // zero-copy interface
        inline UnboundBuffer imbue(size_t len) {
            return {imbueImpl(len)};
        }

        inline void bump(const void* ptr) noexcept {
            bumpImpl(ptr);
        }

        inline void bump(UnboundBuffer buf) noexcept {
            bump(buf.ptr);
        }
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
