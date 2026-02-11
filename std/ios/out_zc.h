#pragma once

#include "output.h"
#include "unbound.h"
#include "outable.h"

#include <std/sys/types.h>

namespace Std {
    class StringView;

    class ZeroCopyOutput: public Output {
        ZeroCopyOutput* zeroCopy() noexcept override;
        size_t writeImpl(const void* data, size_t len) override;

        virtual void* imbueImpl(size_t* len) = 0;
        virtual void commitImpl(const void* ptr) noexcept = 0;

    public:
        ~ZeroCopyOutput() noexcept override;

        // zero-copy interface
        inline UnboundBuffer imbue(size_t len) {
            return {imbueImpl(&len)};
        }

        inline void* imbue(size_t* avail) {
            return imbueImpl(avail);
        }

        inline void commit(const void* ptr) noexcept {
            commitImpl(ptr);
        }

        inline void commit(UnboundBuffer buf) noexcept {
            commit(buf.ptr);
        }
    };

    template <typename O, typename T>
        requires requires(O o) {
            static_cast<ZeroCopyOutput*>(&o);
        }
    inline O&& operator<<(O&& out, const T& t) {
        output<ZeroCopyOutput, T>(out, t);

        return static_cast<O&&>(out);
    }
}
