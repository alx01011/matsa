#include "jtsanGlobals.hpp"
#include "runtime/atomic.hpp"

volatile bool    _is_jtsan_initialized = false;
volatile uint8_t _gc_epoch = 0;

uint8_t get_gc_epoch(void) {
    return Atomic::load(&_gc_epoch);
}

void increment_gc_epoch(void) {
    Atomic::inc((uint8_t*)&_gc_epoch);
}

bool is_jtsan_initialized(void) {
    return Atomic::load(&_is_jtsan_initialized);
}

void set_jtsan_initialized(bool value) {
    Atomic::store(&_is_jtsan_initialized, value);
}