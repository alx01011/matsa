#ifndef JTSAN_GLOBALS_HPP
#define JTSAN_GLOBALS_HPP

bool is_jtsan_initialized(void);
void set_jtsan_initialized(bool value);

void increment_gc_epoch(void);
uint8_t get_gc_epoch(void);

#endif