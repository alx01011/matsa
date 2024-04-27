#include "vectorclock.hpp"

uint64_t& Vectorclock::operator[](size_t index) {
    return this->_clock[index];
}

Vectorclock& Vectorclock::operator=(const Vectorclock& other) {
    for (size_t i = 0; i < other._map_size; i++) {
        if (other._clock[other._map[i]] > this->_clock[other._map[i]]) {
            this->_clock[other._map[i]] = other._clock[other._map[i]];
        }
    }
    return *this;
}

uint64_t Vectorclock::get(size_t index) {
    return this->_clock[index];
}

void Vectorclock::set(size_t index, uint64_t value) {
    this->_clock[index] = value;
    
    // if the index is not in the map, add it
    for (size_t i = 0; i < this->_map_size; i++) {
        if (this->_map[i] == index) {
            return;
        }
    }

    this->_map[this->_map_size++] = index;
}