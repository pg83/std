#pragma once

#include <std/typ/support.h>

namespace stl {
    void xchgPtr(void** l, void** r);

    template <typename T, typename O>
    class RefCountPtr {
        T* t_;

    public:
        RefCountPtr(T* t)
            : t_(t)
        {
            O::ref(t_);
        }

        RefCountPtr(const RefCountPtr& ptr)
            : RefCountPtr(ptr.t_)
        {
        }

        ~RefCountPtr() {
            O::unref(t_);
        }

        RefCountPtr& operator=(const RefCountPtr& r) {
            RefCountPtr(r).xchg(*this);

            return *this;
        }

        template <typename... A>
        static auto make(A&&... a) {
            return RefCountPtr(new T(forward<A>(a)...));
        }

        auto ptr() const {
            return O::ptr(t_);
        }

        auto mutPtr() {
            return O::mutPtr(t_);
        }

        auto refCount() const {
            return t_->refCount();
        }

        // sugar
        auto operator->() {
            return mutPtr();
        }

        auto operator->() const {
            return ptr();
        }

        auto& operator*() {
            return *mutPtr();
        }

        auto& operator*() const {
            return *ptr();
        }

        void xchg(RefCountPtr& r) {
            xchgPtr((void**)&t_, (void**)&r.t_);
        }
    };

    template <typename T>
    struct RefCountOps {
        static auto ref(T* t) {
            t->ref();
        }

        static auto unref(T* t) {
            if (t->refCount() == 1 || t->unref() == 0) {
                delete t;
            }
        }
    };
}
