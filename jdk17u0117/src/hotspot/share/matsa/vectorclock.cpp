#include "vectorclock.hpp"

#include "runtime/os.hpp"

#include <stdio.h>
#include <cstring>


#define max(a, b) ((a) > (b) ? (a) : (b))

Vectorclock::Vectorclock(void) {
    this->_clock = (uint64_t*)os::reserve_memory(MAX_THREADS * sizeof(uint64_t));
    this->_slot  = (uint64_t*)os::reserve_memory(MAX_THREADS * sizeof(uint64_t));
    this->_map   = (uint8_t*)os::reserve_memory(MAX_THREADS * sizeof(uint8_t));

    assert(this->_clock != nullptr, "MATSA: Failed to allocate vector clock memory");
    assert(this->_slot  != nullptr, "MATSA: Failed to allocate vector clock slot memory");
    assert(this->_map   != nullptr, "MATSA: Failed to allocate vector clock map memory");

    bool protect = true;
    protect &= os::protect_memory((char*)this->_clock, MAX_THREADS * sizeof(uint64_t), os::MEM_PROT_RW);
    protect &= os::protect_memory((char*)this->_slot,  MAX_THREADS * sizeof(uint64_t), os::MEM_PROT_RW);
    protect &= os::protect_memory((char*)this->_map,   MAX_THREADS * sizeof(uint8_t), os::MEM_PROT_RW);

    assert(protect, "MATSA: Failed to protect vector clock memory");

    this->_slot_size = 0;
}

Vectorclock::~Vectorclock(void) {
    assert(this->_clock != nullptr, "MATSA: Vector clock memory not allocated");
    assert(this->_slot  != nullptr, "MATSA: Vector clock slot memory not allocated");
    assert(this->_map   != nullptr, "MATSA: Vector clock map memory not allocated");

    os::release_memory((char*)this->_clock, MAX_THREADS * sizeof(uint64_t));
    os::release_memory((char*)this->_slot,  MAX_THREADS * sizeof(uint64_t));
    os::release_memory((char*)this->_map,   MAX_THREADS * sizeof(uint8_t));
}

uint64_t& Vectorclock::operator[](uint64_t index) {
    return this->_clock[index];
} 

Vectorclock& Vectorclock::operator=(const Vectorclock& other) {
    for (uint64_t i = 0; i < other._slot_size; i++) {
        uint64_t index = other._slot[i];

        this->set(index, other._clock[index]);
    }

    return *this;
}

uint64_t Vectorclock::get(uint64_t index) {
    return this->_clock[index];
}

void Vectorclock::set(uint64_t index, uint64_t value) {
    // only apply if the value is greater
    if (this->_clock[index] < value) {
        this->_clock[index] = value;

        if (this->_map[index] == 0) {
            this->_slot[this->_slot_size++] = index;
            this->_map[index] = 1;
        }
    }
}

void Vectorclock::clear(void) {
    // clear every array
    // this only happens when a thread is created to clear previous values
    // this is because threads are reused from a pool

    for (uint64_t i = 0; i < _slot_size; i++) {
        uint64_t index = this->_slot[i];
        this->_clock[index] = 0;
        this->_map[index] = 0;
    }

    this->_slot_size = 0;
}

// unsafe, for debugging purposes only
void Vectorclock::print(void) {
    for (uint64_t i = 0; i < this->_slot_size; i++) {
        uint64_t index = this->_slot[i];
        uint64_t value = this->_clock[index];

        if (value == 0) {
            continue;
        }

        printf("\tThread %lu: %lu\n", index, value);
    }

}

void Vectorclock::release_acquire(Vectorclock* other) {
    for (uint64_t i = 0; i < other->_slot_size; i++) {
        uint64_t index = other->_slot[i];

        other->_clock[index] = max(other->_clock[index], this->_clock[index]);
        this->_clock[index]  = other->_clock[index];
    }
}

void Vectorclock::acquire(Vectorclock* other) {
    for (uint64_t i = 0; i < other->_slot_size; i++) {
        uint64_t index = other->_slot[i];

        this->set(index, other->_clock[index]);
    }
}

void Vectorclock::release(Vectorclock* other) {
    other->acquire(this);
}