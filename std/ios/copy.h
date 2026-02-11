#pragma once

namespace Std {
    class Input;
    class Output;
    class ZeroCopyOutput;

    // dynamic dispatcher
    void copy(Input& in, Output& out);

    void copyIO(Input& in, Output& out);
    void copyIZ(Input& in, ZeroCopyOutput& out);
}
