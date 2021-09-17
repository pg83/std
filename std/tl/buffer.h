#pragma once

#include "support.h"

#include <std/os/types.h>

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

        inline Header* header() const noexcept {
            return (Header*)data_ - 1;
        }

    public:
        ~Buffer();

        Buffer(size_t len);
        Buffer(const void* data, size_t len);

        Buffer() noexcept;
        Buffer(const Buffer& buf);
        Buffer(Buffer&& buf) noexcept;

        inline Buffer& operator=(const Buffer& buf) {
            Buffer(buf).swap(*this);

            return *this;
        }

        inline Buffer& operator=(Buffer&& buf) noexcept {
            Buffer(move(buf)).swap(*this);

            return *this;
        }

        inline void* data() noexcept {
            return data_;
        }

        inline const void* data() const noexcept {
            return data_;
        }

        inline size_t size() const noexcept {
            return header()->size;
        }

        inline size_t used() const noexcept {
            return header()->used;
        }

        inline void swap(Buffer& buf) {
            ::Std::swap(data_, buf.data_);
        }

        void shrinkToFit();
        void grow(size_t size);
        void append(const void* data, size_t len);
    };
}
