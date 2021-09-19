#pragma once

namespace Std {
    template <typename T>
    class IntrusivePtr {
        T* t_;

    public:
        inline IntrusivePtr(T* t) noexcept
            : t_(t)
        {
            t_->ref();
        }

        inline IntrusivePtr(const IntrusivePtr& ptr) noexcept
            : IntrusivePtr(ptr.t_)
        {
        }

        inline ~IntrusivePtr() noexcept {
            if (t_->refCount() == 1 || t_->unref() == 0) {
                delete t_;
            }
        }

        inline const T* ptr() const noexcept {
            return t_;
        }

        inline T* mutPtr() noexcept {
            return t_;
        }

        inline T* operator->() noexcept {
            return mutPtr();
        }

        inline const T* operator->() const noexcept {
            return ptr();
        }
    };
}
