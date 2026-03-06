#pragma once

#include "runable.h"

#include <std/lib/node.h>

namespace stl {
    struct Task: public Runable, public IntrusiveNode {
    };
}
