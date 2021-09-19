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
    };

    class RefCount {
        int counter_ = 0;

    public:
        inline int ref() noexcept {
            return ++counter_;
        }

        inline int refCount() const noexcept {
            return counter_;
        }

        inline int unref() noexcept {
            return --counter_;
        }
    };
}
