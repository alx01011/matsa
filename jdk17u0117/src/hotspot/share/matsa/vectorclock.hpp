#ifndef VECTORCLOCK_HPP
#define VECTORCLOCK_HPP

#include <cstdint>
#include <cstddef>

#include "memory/allocation.hpp"

#include "matsaDefs.hpp"

class Vectorclock : public CHeapObj<mtInternal> {
    private:
        // clock contains the epoch of each thread seen by the current thread
        uint64_t *_clock;
        // slot contains the ids of the threads that the clock has been set
        uint64_t  *_slot;
        size_t   _slot_size;
        // map is used for faster lookup in slot, to determine if a thread has been added
        uint8_t *_map;

    public:
        // default constructor uses MAX_THREADS size
        Vectorclock(void);
        ~Vectorclock(void);


        // overload [] operator to access clock of specific thread
        uint64_t& operator[](uint64_t index);
        // this is essentially the max operation between the two clocks
        Vectorclock& operator=(const Vectorclock& other);
    
        uint64_t get(uint64_t index);
        void set(uint64_t index, uint64_t value);
        // clear (zero out) the clock
        void clear(void);

        void print(void);

        void release_acquire(Vectorclock* other);
        void release(Vectorclock* other);
        void acquire(Vectorclock* other);
};

#endif