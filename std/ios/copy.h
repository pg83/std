#pragma once

namespace Std {
    class Input;
    class Output;
    class ZeroCopyOutput;

    void copy(Input& in, Output& out);
    void zeroCopy(Input& in, ZeroCopyOutput& out);
}
