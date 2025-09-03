#include "vectorclock.hpp"

#include "runtime/os.hpp"

#include <stdio.h>
#include <cstring>

#include <sys/mman.h>
#include <unistd.h>


#define max(a, b) ((a) > (b) ? (a) : (b))

Vectorclock::Vectorclock(void) {
    /*
        TODO:
        Because vectorclocks can be created quite often, protect_memory fails.
        In linux there is a limit close to around 30k invocations.
        I will use raw mmap here and in the future, use mmap across all allocation.
        Preferably, create a custom allocator that calls mmap. 
    */
    this->_clock = (uint64_t*)mmap(NULL, MAX_THREADS * sizeof(uint64_t), PROT_READ | PROT_WRITE,
                                    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    this->_slot  = (uint64_t*)mmap(NULL, MAX_THREADS * sizeof(uint64_t), PROT_READ | PROT_WRITE,
                                    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    this->_map   = (uint8_t*)mmap(NULL, MAX_THREADS * sizeof(uint8_t), PROT_READ | PROT_WRITE,
                                    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    assert((void*)this->_clock != MAP_FAILED, "MATSA: Failed to allocate vector clock memory");
    assert((void*)this->_slot  != MAP_FAILED, "MATSA: Failed to allocate vector clock slot memory");
    assert((void*)this->_map   != MAP_FAILED, "MATSA: Failed to allocate vector clock map memory");

    this->_slot_size = 0;
}

Vectorclock::~Vectorclock(void) {
    assert((void*)this->_clock != MAP_FAILED, "MATSA: Vector clock memory not allocated");
    assert((void*)this->_slot  != MAP_FAILED, "MATSA: Vector clock slot memory not allocated");
    assert((void*)this->_map   != MAP_FAILED, "MATSA: Vector clock map memory not allocated");

    munmap(this->_clock, MAX_THREADS * sizeof(uint64_t));
    munmap(this->_slot,  MAX_THREADS * sizeof(uint64_t));
    munmap(this->_map,   MAX_THREADS * sizeof(uint8_t));
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