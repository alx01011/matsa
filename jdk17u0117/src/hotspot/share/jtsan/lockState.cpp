#include "lockState.hpp"
#include "shadow.hpp"
#include "vectorclock.hpp"

#include "runtime/os.hpp"
#include "runtime/atomic.hpp"

#include <stdio.h>
#include <pthread.h>

#define MAX_LOCKS (100000) // 100k locks should be sufficient

/*
    LockShadow is a singleton class that holds the shadow memory for locks.
    We need 2 instances of LockShadow, one for object locks and one for sync locks.
*/
LockShadow* LockShadow::objectInstance = nullptr;
LockShadow* LockShadow::syncInstance   = nullptr;

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

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
    return (Vectorclock*)((char*)this->addr + (index * sizeof(Vectorclock)));
}

size_t LockShadow::getCurrentLockIndex(void) {
    // because multiple threads (locks) can get an index at the same time
    return Atomic::load(&(this->cur));
}

void LockShadow::incrementLockIndex(void) {
    Atomic::inc(&(this->cur));
}