#pragma once

namespace Std {
    class Input;
    class Output;
    class ZeroCopyOutput;

    // dynamic dispatcher
    void copy(Input& in, Output& out);

    void copyCopy(Input& in, Output& out);
    void zeroCopy(Input& in, ZeroCopyOutput& out);
}
