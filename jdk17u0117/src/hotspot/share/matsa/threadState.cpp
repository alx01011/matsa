#include "threadState.hpp"

#include "runtime/atomic.hpp"
#include "utilities/debug.hpp"
#include "runtime/os.hpp"

#include <string.h>

Vectorclock*   MaTSaThreadState::epoch                = nullptr;
size_t         MaTSaThreadState::size                 = 0;

void MaTSaThreadState::init(void) {
    MaTSaThreadState::size = MAX_THREADS * sizeof(Vectorclock);
    MaTSaThreadState::epoch = new Vectorclock[MAX_THREADS];

    assert(MaTSaThreadState::epoch != nullptr, "MATSA: Failed to allocate thread state memory");
}

void MaTSaThreadState::destroy(void) {
    assert(MaTSaThreadState::epoch != nullptr, "MATSA: Thread state memory not allocated");

    delete[] MaTSaThreadState::epoch;
    MaTSaThreadState::epoch = nullptr;
    MaTSaThreadState::size = 0;
}

Vectorclock* MaTSaThreadState::getThreadState(uint64_t threadId) {
    assert(threadId < MAX_THREADS, "MATSA: Thread id out of bounds");

    return &(MaTSaThreadState::epoch[threadId]);
}

void MaTSaThreadState::incrementEpoch(uint64_t threadId) {
    assert(threadId < MAX_THREADS, "MATSA: Thread id out of bounds");
    
    MaTSaThreadState::epoch[threadId].set(threadId, MaTSaThreadState::epoch[threadId].get(threadId) + 1);
}

void MaTSaThreadState::setEpoch(uint64_t threadId, uint64_t otherThreadId, uint64_t epoch) {
    assert(threadId < MAX_THREADS, "MATSA: Thread id out of bounds");
    assert(otherThreadId < MAX_THREADS, "MATSA: OtherThread id out of bounds");

    MaTSaThreadState::epoch[threadId].set(otherThreadId, epoch);
}

uint64_t MaTSaThreadState::getEpoch(uint64_t threadId, uint64_t otherThreadId) {
    assert(threadId < MaTSaThreadState::size, "MATSA: Thread id out of bounds");
    assert(otherThreadId < MaTSaThreadState::size, "MATSA: Thread id out of bounds");

    return MaTSaThreadState::epoch[threadId].get(otherThreadId);
}

void MaTSaThreadState::transferEpoch(uint64_t from_tid, uint64_t to_tid) {
    assert(from_tid < MaTSaThreadState::size, "MATSA: Thread id out of bounds");
    assert(to_tid < MaTSaThreadState::size, "MATSA: OtherThread id out of bounds");
    
    MaTSaThreadState::epoch[to_tid] = MaTSaThreadState::epoch[from_tid];
}