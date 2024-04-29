#include "lockState.hpp"
#include "shadow.hpp"
#include "vectorclock.hpp"

#include "runtime/os.hpp"
#include "runtime/atomic.hpp"

#include <stdio.h>

/*
    FIXME: 
    MAX_LOCKS should not be hard coded, it could also reach very high values.
    One way to solve this could to clear mappings of locks to clear the shadow memory of oops that have been freed.
*/

#define MAX_LOCKS (1000000000) // 1billion locks should be sufficient this is the equivelant of about 8terabytes of virtual memory

/*
    LockShadow is a singleton class that holds the shadow memory for locks.
    We need 2 instances of LockShadow, one for object locks and one for sync locks.
*/
LockShadow* LockShadow::objectInstance = nullptr;
LockShadow* LockShadow::syncInstance   = nullptr;

LockShadow::LockShadow(void) {
    this->nmemb = MAX_LOCKS;
    this->size = MAX_LOCKS * sizeof(Vectorclock);
    this->cur = 1; // start from 1, as 0 is reserved as invalid index
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

// gets the object lock shadow memory
LockShadow* LockShadow::ObjectLockShadow(void) {
    return LockShadow::objectInstance;
}

// gets the sync lock shadow memory
LockShadow* LockShadow::SyncLockShadow(void) {
    return LockShadow::syncInstance;
}

// initializes the lock shadow memory
void LockShadow::init(void) {
    objectInstance = new LockShadow();
    syncInstance   = new LockShadow();
}

void LockShadow::destroy(void) {
    if (LockShadow::objectInstance != nullptr) {
        delete LockShadow::objectInstance;
    }

    if (LockShadow::syncInstance != nullptr) {
        delete LockShadow::syncInstance;
    }
}

Vectorclock* LockShadow::indexToLockVector(uint32_t index) {
    if (index >= this->nmemb) {
        fprintf(stderr, "Index %u out of bounds of %u\n", index, this->nmemb);
    }

    return (Vectorclock*)((char*)this->addr + (index * sizeof(Vectorclock)));
}

void LockShadow::transferVectorclock(size_t tid, uint32_t index) {
    Vectorclock *vc = this->indexToLockVector(index);
    Vectorclock *threadState = JtsanThreadState::getThreadState(tid);

    *vc = *threadState;
}

uint32_t LockShadow::getCurrentLockIndex(void) {
    // because multiple threads (locks) can get an index at the same time
    return Atomic::load(&(this->cur));
}

void LockShadow::incrementLockIndex(void) {
    Atomic::inc(&(this->cur));
}