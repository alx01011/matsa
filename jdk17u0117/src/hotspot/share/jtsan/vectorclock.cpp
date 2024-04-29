#include "vectorclock.hpp"

#include <stdio.h>

uint64_t& Vectorclock::operator[](size_t index) {
    return this->_clock[index];
}
#include <csignal>
#include <iostream>
void signal_handler(int signal) {
    // dummy
}
 

Vectorclock& Vectorclock::operator=(const Vectorclock& other) {
        std::signal(SIGSEGV, signal_handler);

    for (size_t i = 0; i < other._slot_size; i++) {
        if (other._slot_size >= MAX_THREADS) {
            std::raise(SIGSEGV);
        }

        size_t index = other._slot[i];

        if (index >= MAX_THREADS) {
            fprintf(stderr, "Index %zu out of bounds of 1024\n", index);
        }


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