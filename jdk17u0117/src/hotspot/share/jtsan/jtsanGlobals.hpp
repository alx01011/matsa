#ifndef JTSAN_GLOBALS_HPP
#define JTSAN_GLOBALS_HPP

#include "runtime/atomic.hpp"

bool _is_jtsan_initialized = false;

bool is_jtsan_initialized() {
    return Atomic::load(&_is_jtsan_initialized);
}

void set_jtsan_initialized(bool value) {
    Atomic::store(&_is_jtsan_initialized, value);
}

#endif