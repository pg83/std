#include "context.h"

#include <std/alg/exchange.h>

#include <std/mem/new.h>
#include <ucontext.h>

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

    Context* Context::create(void* buf) noexcept {
        return new (buf) ContextImpl();
    }

    Context* Context::create(void* buf, void* stackPtr, size_t stackSize, void (*fn)(u32, u32), uintptr_t p) noexcept {
        auto* impl = new (buf) ContextImpl();

        getcontext(&impl->uctx);
        impl->uctx.uc_stack.ss_sp = stackPtr;
        impl->uctx.uc_stack.ss_size = stackSize;
        impl->uctx.uc_link = nullptr;
        makecontext(&impl->uctx, (void (*)())fn, 2, (u32)p, (u32)(p >> 32));

        return impl;
    }

    void ContextImpl::switchTo(Context& target) noexcept {
        auto& t = (ContextImpl&)target;
        auto* ehg = __cxxabiv1::__cxa_get_globals();

        ehCaughtExceptions = exchange(ehg->caughtExceptions, t.ehCaughtExceptions);
        ehUncaughtExceptions = exchange(ehg->uncaughtExceptions, t.ehUncaughtExceptions);
        swapcontext(&uctx, &t.uctx);
    }
}
