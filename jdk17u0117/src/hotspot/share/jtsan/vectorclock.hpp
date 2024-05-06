#ifndef VECTORCLOCK_HPP
#define VECTORCLOCK_HPP

#include <cstdint>
#include <cstddef>

#include "jtsanDefs.hpp"

class Vectorclock {
    private:
        // clock contains the epoch of each thread seen by the current thread
        uint64_t _clock[MAX_THREADS];
        // slot contains the ids of the threads that the clock has been set to
        uint8_t  _slot[MAX_THREADS];
        size_t   _slot_size;
        // map is used for faster lookup in slot, to determine if a thread has been added
        uint8_t _map[MAX_THREADS];

    public:
        // overload [] operator to access clock of specific thread
        uint64_t& operator[](size_t index);
        // this is essentially the max operation between the two clocks
        Vectorclock& operator=(const Vectorclock& other);
    
        uint64_t get(size_t index);
        void set(size_t index, uint64_t value);
        // clear (zero out) the clock
        void clear(void);

        void release_acquire(Vectorclock* other);
};

#endif