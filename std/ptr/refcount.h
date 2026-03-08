#pragma once

#include <std/typ/support.h>

namespace stl {
    void xchgPtr(void** l, void** r);

    template <typename T, typename O>
    class RefCountPtr {
        T* t_;

    public:
        RefCountPtr(T* t) noexcept
            : t_(t)
        {
            O::ref(t_);
        }

        RefCountPtr(const RefCountPtr& ptr) noexcept
            : RefCountPtr(ptr.t_)
        {
        }

        ~RefCountPtr() noexcept {
            O::unref(t_);
        }

        RefCountPtr& operator=(const RefCountPtr& r) noexcept {
            RefCountPtr(r).xchg(*this);

            return *this;
        }

        template <typename... A>
        static auto make(A&&... a) {
            return RefCountPtr(new T(forward<A>(a)...));
        }

        auto ptr() const noexcept {
            return O::ptr(t_);
        }

        auto mutPtr() noexcept {
            return O::mutPtr(t_);
        }

        auto refCount() const noexcept {
            return t_->refCount();
        }

        // sugar
        auto operator->() noexcept {
            return mutPtr();
        }

        auto operator->() const noexcept {
            return ptr();
        }

        auto& operator*() noexcept {
            return *mutPtr();
        }

        auto& operator*() const noexcept {
            return *ptr();
        }

        void xchg(RefCountPtr& r) noexcept {
            xchgPtr((void**)&t_, (void**)&r.t_);
        }
    };

    template <typename T>
    struct RefCountOps {
        static auto ref(T* t) noexcept {
            t->ref();
        }

        static auto unref(T* t) noexcept {
            if (t->refCount() == 1 || t->unref() == 0) {
                delete t;
            }
        }
    };
}
