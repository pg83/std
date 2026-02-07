#pragma once

#include <std/mem/new.h>
#include <std/sys/types.h>
#include <std/alg/advance.h>

namespace Std {
    class StringView;

    class Buffer {
        void* data_;

    public:
        struct Header: public Newable {
            size_t used;
            size_t size;
        };

    private:
        inline Header* header() const noexcept {
            return (Header*)data_ - 1;
        }

        void appendUnsafe(const void* data, size_t len);

    public:
        ~Buffer() noexcept;

        Buffer(size_t len);
        Buffer(StringView v);
        Buffer(const void* data, size_t len);

        Buffer() noexcept;
        Buffer(const Buffer& buf);
        Buffer(Buffer&& buf) noexcept;

        inline Buffer& operator=(const Buffer& buf) {
            Buffer(buf).xchg(*this);

            return *this;
        }

        inline Buffer& operator=(Buffer&& buf) noexcept {
            Buffer tmp;

            tmp.xchg(buf);
            tmp.xchg(*this);

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

        inline size_t left() const noexcept {
            return capacity() - used();
        }

        void xchg(Buffer& buf) noexcept;

        inline void reset() noexcept {
            seekAbsolute((size_t)0);
        }

        inline void seekRelative(size_t len) noexcept {
            seekAbsolute(used() + len);
        }

        inline void seekNegative(size_t len) noexcept {
            seekAbsolute(used() - len);
        }

        void seekAbsolute(size_t pos) noexcept;

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
    };
}
