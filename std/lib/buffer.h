#pragma once

#include <std/alg/xchg.h>
#include <std/sys/types.h>
#include <std/typ/support.h>

namespace Std {
    template <typename T>
    inline constexpr T* advancePtr(T* ptr, size_t len) noexcept {
        return (T*)(len + (const u8*)ptr);
    }

    class Buffer {
        void* data_;

    private:
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

        inline auto data() const noexcept {
            return data_;
        }

        inline auto mutData() noexcept {
            return data_;
        }

        inline auto current() const noexcept {
            return advancePtr(data(), used());
        }

        inline auto mutCurrent() noexcept {
            return const_cast<void*>(current());
        }

        inline auto storageEnd() const noexcept {
            return advancePtr(data(), capacity());
        }

        inline auto mutStorageEnd() noexcept {
            return const_cast<void*>(storageEnd());
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

        inline void seekAbsolute(const void* ptr) noexcept {
            seekAbsolute(offsetOf(ptr));
        }

        inline size_t offsetOf(const void* ptr) noexcept {
            return (const u8*)ptr - (const u8*)data();
        }

        void shrinkToFit();
        void grow(size_t size);
        void append(const void* data, size_t len);

        inline void growDelta(size_t len) {
            grow(used() + len);
        }

    private:
        inline Header* header() const noexcept {
            return (Header*)data_ - 1;
        }

        void appendUnsafe(const void* data, size_t len);
    };
}
