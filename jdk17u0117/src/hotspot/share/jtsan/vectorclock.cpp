#include "vectorclock.hpp"

#include <stdio.h>
#include <cstring>

uint64_t& Vectorclock::operator[](size_t index) {
    return this->_clock[index];
} 

Vectorclock& Vectorclock::operator=(const Vectorclock& other) {
    for (size_t i = 0; i < other._slot_size; i++) {
        size_t index = other._slot[i];

        this->set(index, other._clock[index]);
    }

    return *this;
}

uint64_t Vectorclock::get(size_t index) {
    return this->_clock[index];
}

void Vectorclock::set(size_t index, uint64_t value) {
    this->_clock[index] = value;
    
    if (this->_map[index] == 0) {
        // unsafe
        this->_slot[this->_slot_size++] = index;
        this->_map[index] = 1;
    }
}

void Vectorclock::clear(void) {
    if (this->_slot_size == 0) {
        return;
    }

    // clear every array
    // this only happens when a thread is created to clear previous values
    // this is because threads are reused from a pool
    // not a common operation due to the use of a queue
    memset(this->_clock, 0, sizeof(this->_clock));
    memset(this->_slot,  0, sizeof(this->_slot));
    memset(this->_map,   0, sizeof(this->_map));

    this->_slot_size = 0;
}