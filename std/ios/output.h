#pragma once

#include "param.h"

#include <std/sys/types.h>

namespace Std {
    struct Output;

    struct Output {
        virtual ~Output();

        inline void write(const void* data, size_t len) {
            if (len) {
                writeImpl(data, len);
            }
        }

        inline void flush() {
            flushImpl();
        }

        inline void finish() {
            finishImpl();
        }

    private:
        virtual void writeImpl(const void* data, size_t len) = 0;

        // have sensible defaults
        virtual void flushImpl();
        virtual void finishImpl();
    };
}
