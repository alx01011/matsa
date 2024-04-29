#include "vectorclock.hpp"

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