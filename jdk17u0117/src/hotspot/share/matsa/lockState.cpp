#include "lockState.hpp"
#include "shadow.hpp"
#include "vectorclock.hpp"
#include "threadState.hpp"

#include "runtime/os.hpp"
#include "runtime/atomic.hpp"

#include <stdio.h>

/*
    A class for managing the lock shadow memory.
*/
LockShadow::LockShadow(void) {
    this->size          = sizeof(Vectorclock);
    this->addr          = new Vectorclock();
    this->cl_init_addr  = new Vectorclock();
    // this->cl_init_addr  = os::reserve_memory(this->size);
    // bool protect = os::protect_memory((char*)this->addr, this->size, os::MEM_PROT_RW) &&
    //                os::protect_memory((char*)this->cl_init_addr, this->size, os::MEM_PROT_RW);

    // if (this->addr == nullptr || this->cl_init_addr == nullptr || !protect) {
    //     fprintf(stderr, "Failed to allocate lock shadow memory\n");
    //     exit(1);
    // }
}

LockShadow::~LockShadow(void) {
    delete this->addr;
    delete this->cl_init_addr;
    // if (this->addr != nullptr && this->cl_init_addr != nullptr) {
    //     os::release_memory((char*)this->addr, this->size);
    //     os::release_memory((char*)this->cl_init_addr, this->size);
    // }
}

Vectorclock* LockShadow::get_vectorclock(void) {
    return this->addr;
}

Vectorclock* LockShadow::get_cl_init_vectorclock(void) {
    return this->cl_init_addr;
}

void LockShadow::transfer_vc(size_t tid) {
    Vectorclock *vc = this->get_vectorclock();
    Vectorclock *threadState = MaTSaThreadState::getThreadState(tid);

    *vc = *threadState;
}