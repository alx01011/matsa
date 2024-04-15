#ifndef JTSAN_GLOBALS_HPP
#define JTSAN_GLOBALS_HPP

extern bool _is_jtsan_initialized;

bool is_jtsan_initialized(void);

void set_jtsan_initialized(bool value);

#endif