#pragma once

namespace stl {
    struct Mutex;

    class LockGuard {
        Mutex* mutex_;

    public:
        LockGuard(Mutex* m);
        ~LockGuard() noexcept;

        template <typename F>
        auto run(F f) {
            return f();
        }

        void drop() noexcept {
            mutex_ = nullptr;
        }
    };

    class UnlockGuard {
        Mutex* mutex_;

    public:
        UnlockGuard(Mutex* m);
        ~UnlockGuard() noexcept;

        template <typename F>
        auto run(F f) {
            return f();
        }
    };
}
