#pragma once

namespace Std {
    struct Ctx {
        int argc;
        char** argv;

        virtual void printTB() const = 0;

        void run();
    };
}
