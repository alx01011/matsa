#include "jtsanGlobals.hpp"
#include "runtime/atomic.hpp"

volatile bool _is_jtsan_initialized = false;

bool is_jtsan_initialized() {
    return Atomic::load(&_is_jtsan_initialized);
}

void set_jtsan_initialized(bool value) {
    Atomic::store(&_is_jtsan_initialized, value);
}