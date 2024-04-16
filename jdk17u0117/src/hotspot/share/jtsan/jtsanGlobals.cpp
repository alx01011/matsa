#include "jtsanGlobals.hpp"
#include "runtime/atomic.hpp"

#include <cstdint>

volatile bool    _is_jtsan_initialized = false;
volatile uint16_t _gc_epoch = 0;

uint16_t get_gc_epoch(void) {
    return Atomic::load(&_gc_epoch);
}

void increment_gc_epoch(void) {
    Atomic::inc(&_gc_epoch);
}

bool is_jtsan_initialized(void) {
    return Atomic::load(&_is_jtsan_initialized);
}

void set_jtsan_initialized(bool value) {
    Atomic::store(&_is_jtsan_initialized, value);
}