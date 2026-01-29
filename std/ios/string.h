#pragma once

#include "zc_out.h"

namespace Std {
    struct DynString;

    class StringOutput: public ZeroCopyOutput {
        DynString* str_;

        void writeImpl(const void* ptr, size_t len) override;
        void* imbueImpl(size_t len) override;
        void bumpImpl(const void* ptr) noexcept override;
        size_t hintImpl() const noexcept override;

    public:
        ~StringOutput() override;

        inline StringOutput(DynString& str) noexcept
            : str_(&str)
        {
        }

        inline const auto& str() const noexcept {
            return *str_;
        }

        inline auto& mutStr() noexcept {
            return *str_;
        }
    };
}
