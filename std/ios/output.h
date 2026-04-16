#pragma once

#include <std/sys/types.h>

struct iovec;

namespace stl {
    class Input;
    class StringView;
    class ZeroCopyInput;
    class ZeroCopyOutput;

    class Output {
        virtual size_t writeImpl(const void* data, size_t len) = 0;
        virtual size_t hintImpl() const noexcept;

        // have sensible defaults
        virtual void flushImpl();
        virtual void finishImpl();
        virtual size_t writeVImpl(iovec* parts, size_t count);

    public:
        virtual ~Output() noexcept;

        virtual ZeroCopyOutput* upgrade() noexcept;

        void writeC(const void* data, size_t len);

        size_t writeP(const void* data, size_t len);

        size_t writeV(iovec* parts, size_t count);
        size_t writeV(const StringView* parts, size_t count);

        // zero == no hint
        size_t hint() const noexcept {
            return hintImpl();
        }

        bool hint(size_t* res) const noexcept;
        size_t hint(size_t def) const noexcept;

        size_t write(const void* data, size_t len) {
            return (writeC(data, len), len);
        }

        void flush() {
            flushImpl();
        }

        void finish() {
            finishImpl();
        }

        virtual void recvFromI(Input& in);
        virtual void recvFromZ(ZeroCopyInput& in);
    };
}
