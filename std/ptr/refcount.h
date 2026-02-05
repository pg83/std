#pragma once

#include <std/typ/support.h>

namespace Std {
    void xchgPtr(void** l, void** r);

    template <typename T, typename O>
    class RefCountPtr {
        T* t_;

    public:
        inline RefCountPtr(T* t) noexcept
            : t_(t)
        {
            O::ref(t_);
        }

        inline RefCountPtr(const RefCountPtr& ptr) noexcept
            : RefCountPtr(ptr.t_)
        {
        }

        inline ~RefCountPtr() noexcept {
            O::unref(t_);
        }

        inline RefCountPtr& operator=(const RefCountPtr& r) noexcept {
            RefCountPtr(r).xchg(*this);

            return *this;
        }

        template <typename... A>
        static inline auto make(A&&... a) {
            return RefCountPtr(new T(forward<A>(a)...));
        }

        inline auto ptr() const noexcept {
            return O::ptr(t_);
        }

        inline auto mutPtr() noexcept {
            return O::mutPtr(t_);
        }

        inline auto refCount() const noexcept {
            return t_->refCount();
        }

        // sugar
        inline auto operator->() noexcept {
            return mutPtr();
        }

        inline auto operator->() const noexcept {
            return ptr();
        }

        inline auto& operator*() noexcept {
            return *mutPtr();
        }

        inline auto& operator*() const noexcept {
            return *ptr();
        }

        inline void xchg(RefCountPtr& r) noexcept {
            xchgPtr((void**)&t_, (void**)&r.t_);
        }
    };

    template <typename T>
    struct RefCountOps {
        static inline auto ref(T* t) noexcept {
            t->ref();
        }

        static inline auto unref(T* t) noexcept {
            if (t->refCount() == 1 || t->unref() == 0) {
                delete t;
            }
        }
    };
}
