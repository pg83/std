#include "context.h"

#include <std/mem/new.h>
#include <std/dbg/insist.h>
#include <std/thr/runable.h>
#include <std/alg/exchange.h>

using namespace stl;

namespace __cxxabiv1 {
    struct __cxa_eh_globals {
        void* caughtExceptions = nullptr;
        unsigned int uncaughtExceptions = 0;
    };

    extern "C" __cxa_eh_globals* __cxa_get_globals() noexcept;
}

#if defined(__linux__) && defined(__x86_64__)
namespace {
    struct alignas(max_align_t) ContextImpl: public Context, public Newable {
        u64 rsp = 0;
        __cxxabiv1::__cxa_eh_globals ehg_;

        ContextImpl() = default;
        ContextImpl(void* stackPtr, size_t stackSize, Runable& entry) noexcept;

        void switchTo(Context& target) noexcept override;

        __attribute__((naked, noinline))
        static void swapContext(u64*, u64*);

        [[noreturn]]
        static void trampoline();
    };
}

void ContextImpl::swapContext(u64*, u64*) {
    __asm__(
        "pushq %rbx\n\t"
        "pushq %rbp\n\t"
        "pushq %r12\n\t"
        "pushq %r13\n\t"
        "pushq %r14\n\t"
        "pushq %r15\n\t"
        "movq %rsp, (%rdi)\n\t"
        "movq (%rsi), %rsp\n\t"
        "popq %r15\n\t"
        "popq %r14\n\t"
        "popq %r13\n\t"
        "popq %r12\n\t"
        "popq %rbp\n\t"
        "popq %rbx\n\t"
        "retq\n\t"
    );
}

void ContextImpl::trampoline() {
    Runable* r;

    __asm__ volatile("movq %%rbx, %0" : "=r"(r));

    r->run();

    STD_INSIST(false);
}

ContextImpl::ContextImpl(void* stackPtr, size_t stackSize, Runable& entry) noexcept {
    auto* top = (u64*)(((uintptr_t)stackPtr + stackSize) & ~(uintptr_t)15);

    // after swapContext pops 6 regs and does ret (7 * 8 = 56 bytes),
    // rsp = top - 8 which is 8-mod-16, matching the ABI for function entry
    *--top = 0;
    *--top = (u64)trampoline;
    *--top = (u64)&entry; // rbx
    *--top = 0;           // rbp
    *--top = 0;           // r12
    *--top = 0;           // r13
    *--top = 0;           // r14
    *--top = 0;           // r15

    rsp = (u64)top;
}

void ContextImpl::switchTo(Context& target) noexcept {
    auto& t = (ContextImpl&)target;
    ehg_ = exchange(*__cxxabiv1::__cxa_get_globals(), t.ehg_);
    swapContext(&rsp, &t.rsp);
}
#else
#include <ucontext.h>

namespace {
    struct alignas(max_align_t) ContextImpl: public Context, public Newable {
        ucontext_t uctx;
        __cxxabiv1::__cxa_eh_globals ehg_;

        ContextImpl() noexcept;
        ContextImpl(void* stackPtr, size_t stackSize, Runable& entry) noexcept;

        void switchTo(Context& target) noexcept override;

        static void trampoline(u32 lo, u32 hi);
    };
}

ContextImpl::ContextImpl() noexcept {
}

ContextImpl::ContextImpl(void* stackPtr, size_t stackSize, Runable& entry) noexcept {
    getcontext(&uctx);

    uctx.uc_stack.ss_sp = stackPtr;
    uctx.uc_stack.ss_size = stackSize;
    uctx.uc_link = nullptr;

    auto p = (uintptr_t)&entry;

    makecontext(&uctx, (void (*)())trampoline, 2, (u32)p, (u32)(p >> 32));
}

void ContextImpl::trampoline(u32 lo, u32 hi) {
    ((Runable*)(uintptr_t)(((uintptr_t)hi << 32) | lo))->run();
}

void ContextImpl::switchTo(Context& target) noexcept {
    auto& t = (ContextImpl&)target;
    ehg_ = exchange(*__cxxabiv1::__cxa_get_globals(), t.ehg_);
    swapcontext(&uctx, &t.uctx);
}
#endif

size_t Context::implSize() noexcept {
    static_assert(sizeof(ContextImpl) % sizeof(max_align_t) == 0);

    return sizeof(ContextImpl);
}

Context* Context::create(void* buf) noexcept {
    return new (buf) ContextImpl();
}

Context* Context::create(void* buf, void* stackPtr, size_t stackSize, Runable& entry) noexcept {
    return new (buf) ContextImpl(stackPtr, stackSize, entry);
}
