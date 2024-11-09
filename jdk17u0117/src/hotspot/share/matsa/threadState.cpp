#include "threadState.hpp"


#include "runtime/atomic.hpp"
#include "utilities/debug.hpp"
#include "runtime/os.hpp"

#include <string.h>

Vectorclock*   MaTSaThreadState::epoch                = nullptr;
size_t         MaTSaThreadState::size                 = 0;

void MaTSaThreadState::init(void) {
    MaTSaThreadState::size = MAX_THREADS * sizeof(Vectorclock);

    MaTSaThreadState::epoch = (Vectorclock*)os::reserve_memory(MaTSaThreadState::size);

    if (MaTSaThreadState::epoch == nullptr) {
        fprintf(stderr, "MATSA: Failed to allocate thread state memory\n");
        exit(1);
    }

    bool protect = os::protect_memory((char*)MaTSaThreadState::epoch, MaTSaThreadState::size, os::MEM_PROT_RW);
    if (!protect) {
        fprintf(stderr, "MATSA: Failed to protect thread state\n");
        exit(1);
    }

    for (size_t i = 0; i < MAX_THREADS; i++) {
        MaTSaThreadState::history[i] = new ThreadHistory();
    }
}

void MaTSaThreadState::destroy(void) {
    if (MaTSaThreadState::epoch != nullptr) {
        os::release_memory((char*)MaTSaThreadState::epoch, MaTSaThreadState::size);
    }

    MaTSaThreadState::epoch = nullptr;
    MaTSaThreadState::size = 0;
}

Vectorclock* MaTSaThreadState::getThreadState(size_t threadId) {
    assert(threadId < MAX_THREADS, "MATSA: Thread id out of bounds");

    return &(MaTSaThreadState::epoch[threadId]);
}

void MaTSaThreadState::incrementEpoch(size_t threadId) {
    assert(threadId < MAX_THREADS, "MATSA: Thread id out of bounds");
    
    MaTSaThreadState::epoch[threadId].set(threadId, MaTSaThreadState::epoch[threadId].get(threadId) + 1);
}

void MaTSaThreadState::setEpoch(size_t threadId, size_t otherThreadId, uint32_t epoch) {
    assert(threadId < MAX_THREADS, "MATSA: Thread id out of bounds");
    assert(otherThreadId < MAX_THREADS, "MATSA: OtherThread id out of bounds");

    MaTSaThreadState::epoch[threadId].set(otherThreadId, epoch);
}

uint32_t MaTSaThreadState::getEpoch(size_t threadId, size_t otherThreadId) {
    assert(threadId < MaTSaThreadState::size, "MATSA: Thread id out of bounds");
    assert(otherThreadId < MaTSaThreadState::size, "MATSA: Thread id out of bounds");

    return MaTSaThreadState::epoch[threadId].get(otherThreadId);
}

void MaTSaThreadState::transferEpoch(size_t from_tid, size_t to_tid) {
    assert(from_tid < MaTSaThreadState::size, "MATSA: Thread id out of bounds");
    assert(to_tid < MaTSaThreadState::size, "MATSA: OtherThread id out of bounds");
    
    MaTSaThreadState::epoch[to_tid] = MaTSaThreadState::epoch[from_tid];
}