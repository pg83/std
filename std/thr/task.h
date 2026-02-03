#pragma once

#include "runable.h"

#include <std/lib/node.h>

namespace Std {
    struct Task: public Runable, public IntrusiveNode {
    };
}
