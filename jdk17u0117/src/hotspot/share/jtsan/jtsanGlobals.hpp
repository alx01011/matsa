#ifndef JTSAN_GLOBALS_HPP
#define JTSAN_GLOBALS_HPP

#include <cstdint>

#include "runtime/mutex.hpp"
#include "runtime/atomic.hpp"

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

#define COUNTER(x)\
    static uint64_t x##_counter;\
    static void increment_##x(void) {\
        x##_counter++;\
    }\
    static uint64_t get_##x(void) {\
        return x##_counter;\
    }
    
#define COUNTER_INC(x)\
    JTSanStats::increment_##x()

#define COUNTER_GET(x)\
    JTSanStats::get_##x()

class JTSanStats {
    public:
        COUNTER(race);
};

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

// check for x86
if defined(__x86_64__) || defined(__i386__)
#define CPU_PAUSE() asm volatile("pause\n": : :"memory")
#else
#define CPU_PAUSE()
#endif

class JTSanSpinLock {
    public:
        JTSanSpinLock(uint8_t *lock) : _lock(lock) {
            while (1) {
                // cmpxchg (dest, compare, exchange)
                if (Atomic::cmpxchg(_lock, 0, 1) == 0) {
                    break;
                }

                while (*_lock) {
                    CPU_PAUSE();
                }
            }
        }

        ~JTSanSpinLock() {
            *_lock = 0;
        }

    private:
        uint8_t *_lock;
};

#endif