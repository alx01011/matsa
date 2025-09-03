#ifndef MATSA_GLOBALS_HPP
#define MATSA_GLOBALS_HPP

#include <cstdint>

#include "runtime/mutex.hpp"

// this gets set up on init and is fetched from MATSA_HISTORY env var
extern uint64_t env_event_buffer_size;
extern uint64_t matsa_history_size;

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

void matsa_at_exit_handler(void);

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