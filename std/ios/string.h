#pragma once

#include "output.h"

namespace Std {
    struct DynString;

    class StringOutput: public ZeroCopyOutput {
        DynString* str_;

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

    private:
        void writeImpl(const void* ptr, size_t len) override;
        void* imbueImpl(size_t len) override;
        void bumpImpl(const void* ptr) noexcept override;
    };
}
