#include "lockState.hpp"
#include "shadow.hpp"
#include "vectorclock.hpp"

#include "runtime/os.hpp"
#include "runtime/atomic.hpp"

#include <stdio.h>

/*
    A class for managing the lock shadow memory.
*/
LockShadow::LockShadow(void) {
    this->size = sizeof(Vectorclock);
    this->addr = os::reserve_memory(this->size);
    bool protect = os::protect_memory((char*)this->addr, this->size, os::MEM_PROT_RW);

    if (this->addr == nullptr || !protect) {
        fprintf(stderr, "Failed to allocate lock shadow memory\n");
        exit(1);
    }
}

LockShadow::~LockShadow(void) {
    if (this->addr != nullptr) {
        os::release_memory((char*)this->addr, this->size);
    }
}

Vectorclock* LockShadow::get_vectorclock(void) {
    return (Vectorclock*)this->addr;
}

void LockShadow::transfer_vc(size_t tid) {
    Vectorclock *vc = this->get_vectorclock();
    Vectorclock *threadState = JTSanThreadState::getThreadState(tid);

    *vc = *threadState;
}