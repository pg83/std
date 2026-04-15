#include "cond_var_iface.h"

using namespace stl;

CondVarIface::~CondVarIface() noexcept {
}

bool CondVarIface::owned() const noexcept {
    return false;
}
