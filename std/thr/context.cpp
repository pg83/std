#include "context.h"

#include <std/mem/new.h>
#include <std/dbg/insist.h>
#include <std/thr/runable.h>
#include <std/alg/exchange.h>

using namespace stl;

namespace {
#if __has_feature(address_sanitizer) || defined(__SANITIZE_ADDRESS__)
    extern "C" {
        void __sanitizer_start_switch_fiber(void** fakeStackSave, const void* bottom, size_t size);
        void __sanitizer_finish_switch_fiber(void* fakeStackSave, const void** bottomOld, size_t* sizeOld);
    }

    struct AsanFiberState {
        void* fakeStack_ = nullptr;
        AsanFiberState* prev_ = nullptr;
        const void* stackBottom_ = nullptr;
        size_t stackSize_ = 0;

        void initStack(void* ptr, size_t size) noexcept {
            stackBottom_ = ptr;
            stackSize_ = size;
        }

        void beforeSwitch(AsanFiberState& target) noexcept {
            __sanitizer_start_switch_fiber(&fakeStack_, target.stackBottom_, target.stackSize_);
            target.prev_ = this;
        }

        void finishSwitch() noexcept {
            __sanitizer_finish_switch_fiber(fakeStack_, &stackBottom_, &stackSize_);
        }

        void afterSwitch() noexcept {
            prev_->finishSwitch();
        }
    };
#else
    struct AsanFiberState {
        void initStack(void*, size_t) noexcept {
        }

        void beforeSwitch(AsanFiberState&) noexcept {
        }

        void afterSwitch() noexcept {
        }
    };
#endif
}

namespace __cxxabiv1 {
    struct __cxa_eh_globals {
        void* caughtExceptions = nullptr;
        unsigned int uncaughtExceptions = 0;
    };

    extern "C" __cxa_eh_globals* __cxa_get_globals() noexcept;
}

namespace {
    struct alignas(max_align_t) ContextBase: public Context, public Newable {
        static void operator delete(void*) noexcept {
        }
    };
}

#if defined(__x86_64__)
namespace {
    struct ContextImpl: public ContextBase, public AsanFiberState {
        u64 rsp = 0;
        __cxxabiv1::__cxa_eh_globals ehg_;
        Runable* entry_ = nullptr;

        ContextImpl() = default;
        ContextImpl(void* stackPtr, size_t stackSize, Runable& entry) noexcept;

        void switchTo(Context& target) noexcept override;

        __attribute__((naked, noinline)) static void swapContext(u64*, u64*);

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
        "retq\n\t");
}

__attribute__((no_sanitize("address"))) void ContextImpl::trampoline() {
    ContextImpl* self;

    __asm__ volatile("movq %%rbx, %0" : "=r"(self));
    self->afterSwitch();

    self->entry_->run();

    STD_INSIST(false);
}

ContextImpl::ContextImpl(void* stackPtr, size_t stackSize, Runable& entry) noexcept {
    entry_ = &entry;

    auto* top = (u64*)(((uintptr_t)stackPtr + stackSize) & ~(uintptr_t)15);

    // after swapContext pops 6 regs and does ret (7 * 8 = 56 bytes),
    // rsp = top - 8 which is 8-mod-16, matching the ABI for function entry
    *--top = 0;
    *--top = (u64)trampoline;
    *--top = (u64)this; // rbx
    *--top = 0;         // rbp
    *--top = 0;         // r12
    *--top = 0;         // r13
    *--top = 0;         // r14
    *--top = 0;         // r15

    rsp = (u64)top;
    initStack(stackPtr, stackSize);
}

void ContextImpl::switchTo(Context& target) noexcept {
    auto& t = (ContextImpl&)target;
    ehg_ = exchange(*__cxxabiv1::__cxa_get_globals(), t.ehg_);
    beforeSwitch(t);
    swapContext(&rsp, &t.rsp);
    afterSwitch();
}
#elif defined(__aarch64__)
namespace {
    struct ContextImpl: public ContextBase, public AsanFiberState {
        u64 sp_ = 0;
        __cxxabiv1::__cxa_eh_globals ehg_;
        Runable* entry_ = nullptr;

        ContextImpl() = default;
        ContextImpl(void* stackPtr, size_t stackSize, Runable& entry) noexcept;

        void switchTo(Context& target) noexcept override;

        __attribute__((naked, noinline)) static void swapContext(u64*, u64*);

        [[noreturn]]
        static void trampoline();
    };
}

void ContextImpl::swapContext(u64*, u64*) {
    __asm__(
        "stp d8,  d9,  [sp, #-16]!\n\t"
        "stp d10, d11, [sp, #-16]!\n\t"
        "stp d12, d13, [sp, #-16]!\n\t"
        "stp d14, d15, [sp, #-16]!\n\t"
        "stp x19, x20, [sp, #-16]!\n\t"
        "stp x21, x22, [sp, #-16]!\n\t"
        "stp x23, x24, [sp, #-16]!\n\t"
        "stp x25, x26, [sp, #-16]!\n\t"
        "stp x27, x28, [sp, #-16]!\n\t"
        "stp x29, x30, [sp, #-16]!\n\t"
        "mov x2, sp\n\t"
        "str x2, [x0]\n\t"
        "ldr x2, [x1]\n\t"
        "mov sp, x2\n\t"
        "ldp x29, x30, [sp], #16\n\t"
        "ldp x27, x28, [sp], #16\n\t"
        "ldp x25, x26, [sp], #16\n\t"
        "ldp x23, x24, [sp], #16\n\t"
        "ldp x21, x22, [sp], #16\n\t"
        "ldp x19, x20, [sp], #16\n\t"
        "ldp d14, d15, [sp], #16\n\t"
        "ldp d12, d13, [sp], #16\n\t"
        "ldp d10, d11, [sp], #16\n\t"
        "ldp d8,  d9,  [sp], #16\n\t"
        "ret\n\t");
}

__attribute__((no_sanitize("address"))) void ContextImpl::trampoline() {
    ContextImpl* self;

    __asm__ volatile("mov %0, x19" : "=r"(self));
    self->afterSwitch();

    self->entry_->run();

    STD_INSIST(false);
}

ContextImpl::ContextImpl(void* stackPtr, size_t stackSize, Runable& entry) noexcept {
    entry_ = &entry;

    auto* top = (u64*)(((uintptr_t)stackPtr + stackSize) & ~(uintptr_t)15);

    // *--top fills high-to-low; ldp pops low-to-high
    // last pushed = lowest address = first popped
    // d8, d9 (popped last, filled first)
    *--top = 0; // d9
    *--top = 0; // d8
    // d10, d11
    *--top = 0; // d11
    *--top = 0; // d10
    // d12, d13
    *--top = 0; // d13
    *--top = 0; // d12
    // d14, d15
    *--top = 0; // d15
    *--top = 0; // d14
    // x19, x20
    *--top = 0;         // x20
    *--top = (u64)this; // x19 — ContextImpl*
    // x21, x22
    *--top = 0; // x22
    *--top = 0; // x21
    // x23, x24
    *--top = 0; // x24
    *--top = 0; // x23
    // x25, x26
    *--top = 0; // x26
    *--top = 0; // x25
    // x27, x28
    *--top = 0; // x28
    *--top = 0; // x27
    // x29, x30 (popped first, filled last)
    *--top = (u64)trampoline; // x30 (lr)
    *--top = 0;               // x29 (fp)

    sp_ = (u64)top;
    initStack(stackPtr, stackSize);
}

void ContextImpl::switchTo(Context& target) noexcept {
    auto& t = (ContextImpl&)target;
    ehg_ = exchange(*__cxxabiv1::__cxa_get_globals(), t.ehg_);
    beforeSwitch(t);
    swapContext(&sp_, &t.sp_);
    afterSwitch();
}
#else
    #include <ucontext.h>

namespace {
    struct ContextImpl: public ContextBase {
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
    static_assert(sizeof(ContextImpl) % alignof(max_align_t) == 0);

    return sizeof(ContextImpl);
}

Context* Context::create(void* buf) noexcept {
    return new (buf) ContextImpl();
}

Context* Context::create(void* buf, void* stackPtr, size_t stackSize, Runable& entry) noexcept {
    return new (buf) ContextImpl(stackPtr, stackSize, entry);
}

Context::~Context() {
}
