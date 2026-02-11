#pragma once

namespace Std {
    class Input;
    class Output;
    class ZeroCopyInput;
    class ZeroCopyOutput;

    void copyIO(Input& in, Output& out);
    void copyIZ(Input& in, ZeroCopyOutput& out);
    void copyZO(ZeroCopyInput& in, Output& out);
    void copyZZ(ZeroCopyInput& in, ZeroCopyOutput& out);
}
