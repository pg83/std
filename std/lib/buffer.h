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
        Header* header() const {
            return (Header*)data_ - 1;
        }

        void appendUnsafe(const void* data, size_t len);

    public:
        ~Buffer();

        Buffer(size_t len);
        Buffer(StringView v);
        Buffer(const void* data, size_t len);

        Buffer();
        Buffer(const Buffer& buf);
        Buffer(Buffer&& buf);

        Buffer& operator=(const Buffer& buf) {
            Buffer(buf).xchg(*this);

            return *this;
        }

        Buffer& operator=(Buffer&& buf) {
            Buffer tmp;

            tmp.xchg(buf);
            tmp.xchg(*this);

            return *this;
        }

        auto data() const {
            return data_;
        }

        auto mutData() {
            return data_;
        }

        auto current() const {
            return advancePtr(data(), used());
        }

        auto mutCurrent() {
            return const_cast<void*>(current());
        }

        auto storageEnd() const {
            return advancePtr(data(), capacity());
        }

        auto mutStorageEnd() {
            return const_cast<void*>(storageEnd());
        }

        size_t capacity() const {
            return header()->size;
        }

        size_t used() const {
            return header()->used;
        }

        size_t length() const {
            return used();
        }

        bool empty() const {
            return used() == 0;
        }

        size_t left() const {
            return capacity() - used();
        }

        void xchg(Buffer& buf);

        void reset() {
            seekAbsolute((size_t)0);
        }

        void seekRelative(size_t len) {
            seekAbsolute(used() + len);
        }

        void seekNegative(size_t len) {
            seekAbsolute(used() - len);
        }

        void seekAbsolute(size_t pos);

        void seekAbsolute(const void* ptr) {
            seekAbsolute(offsetOf(ptr));
        }

        size_t offsetOf(const void* ptr) {
            return (const u8*)ptr - (const u8*)data();
        }

        void shrinkToFit();
        void grow(size_t size);
        void setCapacity(size_t cap);
        void append(const void* data, size_t len);

        void growDelta(size_t len) {
            grow(used() + len);
        }

        void* imbueMe(size_t* len);

        char* cStr();
        void zero(size_t len);
    };
}
