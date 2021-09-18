#pragma once

#include "support.h"

#include <std/alg/xchg.h>
#include <std/sys/types.h>

namespace Std {
    class Buffer {
        void* data_;

        struct Header {
            size_t used;
            size_t size;

            inline Header(size_t len) noexcept
                : used(0)
                , size(len)
            {
            }

            static Header* null() noexcept;
            static Header* alloc(size_t len);
            static void free(Header* ptr) noexcept;
        };

    public:
        ~Buffer();

        Buffer(size_t len);
        Buffer(const void* data, size_t len);

        Buffer() noexcept;
        Buffer(const Buffer& buf);
        Buffer(Buffer&& buf) noexcept;

        inline Buffer& operator=(const Buffer& buf) {
            Buffer(buf).xchg(*this);

            return *this;
        }

        inline Buffer& operator=(Buffer&& buf) noexcept {
            Buffer(move(buf)).xchg(*this);

            return *this;
        }

        inline void* data() noexcept {
            return data_;
        }

        inline const void* data() const noexcept {
            return data_;
        }

        inline size_t capacity() const noexcept {
            return header()->size;
        }

        inline size_t used() const noexcept {
            return header()->used;
        }

        inline size_t length() const noexcept {
            return used();
        }

        inline bool empty() const noexcept {
            return used() == 0;
        }

        inline bool left() const noexcept {
            return capacity() - used();
        }

        inline void xchg(Buffer& buf) {
            ::Std::xchg(data_, buf.data_);
        }

        inline void reset() noexcept {
            header()->used = 0;
        }

        inline void seekRelative(size_t len) noexcept {
            seekAbsolute(used() + len);
        }

        inline void seekAbsolute(size_t pos) noexcept {
            header()->used = pos;
        }

        void shrinkToFit();
        void grow(size_t size);
        void append(const void* data, size_t len);

    private:
        inline Header* header() const noexcept {
            return (Header*)data_ - 1;
        }

        void appendUnsafe(const void* data, size_t len);
    };
}
