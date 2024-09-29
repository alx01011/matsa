#include "matsaGlobals.hpp"
#include "runtime/atomic.hpp"

#include <cstdio>

uint64_t env_event_buffer_size = 1 << 17;

volatile bool    _is_matsa_initialized = false;
uint32_t _gc_epoch = 0;

volatile bool   _is_klass_init = false;

// start from 1 since 0 is reserved for the initial java thread
volatile uint16_t _cur_tid = 1;

uint32_t get_gc_epoch(void) {
    return Atomic::load(&_gc_epoch);
}

void increment_gc_epoch(void) {
    uint32_t epoch = Atomic::load(&_gc_epoch);
    Atomic::store(&_gc_epoch, (uint32_t)(epoch + 1));

}

bool is_matsa_initialized(void) {
    return Atomic::load(&_is_matsa_initialized);
}

void set_matsa_initialized(bool value) {
    Atomic::store(&_is_matsa_initialized, value);
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

uint16_t get_tid(void) {
    return Atomic::load(&_cur_tid);
}

void increment_tid(void) {
    uint16_t tid = Atomic::load(&_cur_tid);
    Atomic::store(&_cur_tid, (uint16_t)(tid + 1));
}

void decrement_tid(void) {
    uint16_t tid = Atomic::load(&_cur_tid);
    Atomic::store(&_cur_tid, (uint16_t)(tid - 1));
}

#define COUNTER_DEF(x)\
    uint64_t MaTSaStats::x##_counter = 0;

COUNTER_DEF(race);