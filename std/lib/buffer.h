#pragma once

#include <std/mem/new.h>
#include <std/sys/types.h>
#include <std/alg/advance.h>

namespace stl {
    class StringView;

    class Buffer {
        void* data_;

    public:
        struct Header: public Newable {
            size_t used;
            size_t size;
        };

    private:
        Header* header() const noexcept {
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

        Buffer& operator=(const Buffer& buf) {
            Buffer(buf).xchg(*this);

            return *this;
        }

        Buffer& operator=(Buffer&& buf) noexcept {
            Buffer tmp;

            tmp.xchg(buf);
            tmp.xchg(*this);

            return *this;
        }

        auto data() const noexcept {
            return data_;
        }

        auto mutData() noexcept {
            return data_;
        }

        auto current() const noexcept {
            return advancePtr(data(), used());
        }

        auto mutCurrent() noexcept {
            return const_cast<void*>(current());
        }

        auto storageEnd() const noexcept {
            return advancePtr(data(), capacity());
        }

        auto mutStorageEnd() noexcept {
            return const_cast<void*>(storageEnd());
        }

        size_t capacity() const noexcept {
            return header()->size;
        }

        size_t used() const noexcept {
            return header()->used;
        }

        size_t length() const noexcept {
            return used();
        }

        bool empty() const noexcept {
            return used() == 0;
        }

        size_t left() const noexcept {
            return capacity() - used();
        }

        void xchg(Buffer& buf) noexcept;

        void reset() noexcept {
            seekAbsolute((size_t)0);
        }

        void seekRelative(size_t len) noexcept {
            seekAbsolute(used() + len);
        }

        void seekNegative(size_t len) noexcept {
            seekAbsolute(used() - len);
        }

        void seekAbsolute(size_t pos) noexcept;

        void seekAbsolute(const void* ptr) noexcept {
            seekAbsolute(offsetOf(ptr));
        }

        size_t offsetOf(const void* ptr) noexcept {
            return (const u8*)ptr - (const u8*)data();
        }

        void shrinkToFit();
        void grow(size_t size);
        void setCapacity(size_t cap) noexcept;
        void append(const void* data, size_t len);

        void growDelta(size_t len) {
            grow(used() + len);
        }

        void* imbueMe(size_t* len);

        char* cStr();
        void zero(size_t len);
    };
}
