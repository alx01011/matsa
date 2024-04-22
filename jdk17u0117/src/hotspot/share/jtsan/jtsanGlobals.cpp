#include "jtsanGlobals.hpp"
#include "runtime/atomic.hpp"

volatile bool    _is_jtsan_initialized = false;
volatile unsigned char _gc_epoch = 1;

volatile bool   _is_klass_init = false;

unsigned char get_gc_epoch(void) {
    return Atomic::load(&_gc_epoch);
}

void increment_gc_epoch(void) {
    uint8_t epoch = Atomic::load(&_gc_epoch);
    Atomic::store(&_gc_epoch, (unsigned char)(epoch + 1));
}

bool is_jtsan_initialized(void) {
    return Atomic::load(&_is_jtsan_initialized);
}

void set_jtsan_initialized(bool value) {
    Atomic::store(&_is_jtsan_initialized, value);
}

void clear_klass_init(void) {
    Atomic::store(&_is_klass_init, false);
}
void set_klass_init(void) {
    Atomic::store(&_is_klass_init, true);
}

bool is_klass_init(void) {
    return Atomic::load(&_is_klass_init);
}