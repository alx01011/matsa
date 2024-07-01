#ifndef JTSAN_GLOBALS_HPP
#define JTSAN_GLOBALS_HPP

#include <cstdint>

#include "runtime/mutex.hpp"

bool is_jtsan_initialized(void);
void set_jtsan_initialized(bool value);

void          increment_gc_epoch(void);
uint32_t      get_gc_epoch(void);

void clear_klass_init(void);
void set_klass_init(void);
bool is_klass_init(void);

uint16_t get_tid(void);
void     increment_tid(void);
void     decrement_tid(void);

class JTSanScopedLock {
    public:
        JTSanScopedLock(Mutex *lock) : _lock(lock) {
            _lock->lock();
        }

        ~JTSanScopedLock() {
            _lock->unlock();
        }
    private:
        Mutex *_lock;
};

#endif