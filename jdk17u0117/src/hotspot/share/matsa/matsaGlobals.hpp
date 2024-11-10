#ifndef MATSA_GLOBALS_HPP
#define MATSA_GLOBALS_HPP

#include <cstdint>

#include "runtime/mutex.hpp"
#include "runtime/atomic.hpp"

extern uint64_t matsa_history_size;

bool is_matsa_initialized(void);
void set_matsa_initialized(bool value);

void          increment_gc_epoch(void);
uint32_t      get_gc_epoch(void);

void clear_klass_init(void);
void set_klass_init(void);
bool is_klass_init(void);

uint16_t get_tid(void);
void     increment_tid(void);
void     decrement_tid(void);

#define COUNTER(x)\
    static uint64_t x##_counter;\
    static void increment_##x(void) {\
        x##_counter++;\
    }\
    static uint64_t get_##x(void) {\
        return x##_counter;\
    }
    
#define COUNTER_INC(x)\
    MaTSaStats::increment_##x()

#define COUNTER_GET(x)\
    MaTSaStats::get_##x()

class MaTSaStats {
    public:
        COUNTER(race);
};

class MaTSaScopedLock {
    public:
        MaTSaScopedLock(Mutex *lock) : _lock(lock) {
            _lock->lock();
        }

        ~MaTSaScopedLock() {
            _lock->unlock();
        }
    private:
        Mutex *_lock;
};

// check for x86
#if defined(__x86_64__) || defined(__i386__)
#define CPU_PAUSE() asm volatile("pause\n": : :"memory")
#else
#define CPU_PAUSE()
#endif

class MaTSaSpinLock {
    public:
        MaTSaSpinLock(uint8_t *lock) : _lock(lock) {
            while (1) {
                // cmpxchg (dest, compare, exchange)
                if (Atomic::cmpxchg(_lock, (uint8_t)0, (uint8_t)1) == 0) {
                    break;
                }

                while (*_lock) {
                    CPU_PAUSE();
                }
            }
        }

        ~MaTSaSpinLock() {
            *_lock = 0;
        }

    private:
        uint8_t *_lock;
};

#endif