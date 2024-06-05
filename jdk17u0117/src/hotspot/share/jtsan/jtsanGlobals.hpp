#ifndef JTSAN_GLOBALS_HPP
#define JTSAN_GLOBALS_HPP

#include <cstdint>

bool is_jtsan_initialized(void);
void set_jtsan_initialized(bool value);

void          increment_gc_epoch(void);
unsigned char get_gc_epoch(void);

void clear_klass_init(void);
void set_klass_init(void);
bool is_klass_init(void);

uint16_t get_tid(void);
void     increment_tid(void);
void     decrement_tid(void);

#endif