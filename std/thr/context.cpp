#include "context.h"

#include <std/alg/exchange.h>
#include <std/thr/runable.h>

#include <std/mem/new.h>
#include <ucontext.h>

using namespace stl;

namespace __cxxabiv1 {
    struct __cxa_eh_globals {
        void* caughtExceptions;
        unsigned int uncaughtExceptions;
    };

    extern "C" __cxa_eh_globals* __cxa_get_globals() noexcept;
}

namespace stl {
    struct ContextImpl: public Context, public Newable {
        ucontext_t uctx;
        void* ehCaughtExceptions = nullptr;
        unsigned int ehUncaughtExceptions = 0;

        void switchTo(Context& target) noexcept override;
    };

    static_assert(sizeof(ContextImpl) <= Context::kBufSize);
}

Context* Context::create(void* buf) noexcept {
    return new (buf) ContextImpl();
}

static void runableTrampoline(u32 lo, u32 hi) {
    ((Runable*)(uintptr_t)(((uintptr_t)hi << 32) | lo))->run();
}

Context* Context::create(void* buf, void* stackPtr, size_t stackSize, Runable& entry) noexcept {
    auto* impl = new (buf) ContextImpl();
    auto p = (uintptr_t)&entry;

    getcontext(&impl->uctx);
    impl->uctx.uc_stack.ss_sp = stackPtr;
    impl->uctx.uc_stack.ss_size = stackSize;
    impl->uctx.uc_link = nullptr;
    makecontext(&impl->uctx, (void (*)())runableTrampoline, 2, (u32)p, (u32)(p >> 32));

    return impl;
}

void ContextImpl::switchTo(Context& target) noexcept {
    auto& t = (ContextImpl&)target;
    auto* ehg = __cxxabiv1::__cxa_get_globals();

    ehCaughtExceptions = exchange(ehg->caughtExceptions, t.ehCaughtExceptions);
    ehUncaughtExceptions = exchange(ehg->uncaughtExceptions, t.ehUncaughtExceptions);
    swapcontext(&uctx, &t.uctx);
}
