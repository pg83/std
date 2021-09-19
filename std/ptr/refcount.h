#pragma once

namespace Std {
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

        static inline auto make() {
            return RefCountPtr(new T());
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
    };
}
