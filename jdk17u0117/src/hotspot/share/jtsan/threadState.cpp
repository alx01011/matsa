#include "threadState.hpp"


#include "runtime/atomic.hpp"
#include "utilities/debug.hpp"
#include "runtime/os.hpp"

#include <string.h>

Vectorclock*   JTSanThreadState::epoch                = nullptr;
size_t         JTSanThreadState::size                 = 0;
ThreadHistory* JTSanThreadState::history[MAX_THREADS] = {nullptr};

void JTSanThreadState::init(void) {
    JTSanThreadState::size = MAX_THREADS * sizeof(Vectorclock);

    JTSanThreadState::epoch = (Vectorclock*)os::reserve_memory(JTSanThreadState::size);

    if (JTSanThreadState::epoch == nullptr) {
        fprintf(stderr, "JTSAN: Failed to allocate thread state memory\n");
        exit(1);
    }

    bool protect = os::protect_memory((char*)JTSanThreadState::epoch, JTSanThreadState::size, os::MEM_PROT_RW);
    if (!protect) {
        fprintf(stderr, "JTSAN: Failed to protect thread state\n");
        exit(1);
    }

    for (size_t i = 0; i < MAX_THREADS; i++) {
        JTSanThreadState::history[i] = new ThreadHistory();
    }
}

void JTSanThreadState::destroy(void) {
    if (JTSanThreadState::epoch != nullptr) {
        os::release_memory((char*)JTSanThreadState::epoch, JTSanThreadState::size);
    }

    JTSanThreadState::epoch = nullptr;
    JTSanThreadState::size = 0;
}

Vectorclock* JTSanThreadState::getThreadState(size_t threadId) {
    assert(threadId < MAX_THREADS, "JTSAN: Thread id out of bounds");

    return &(JTSanThreadState::epoch[threadId]);
}

void JTSanThreadState::incrementEpoch(size_t threadId) {
    assert(threadId < MAX_THREADS, "JTSAN: Thread id out of bounds");
    
    JTSanThreadState::epoch[threadId].set(threadId, JTSanThreadState::epoch[threadId].get(threadId) + 1);
}

void JTSanThreadState::setEpoch(size_t threadId, size_t otherThreadId, uint32_t epoch) {
    assert(threadId < MAX_THREADS, "JTSAN: Thread id out of bounds");
    assert(otherThreadId < MAX_THREADS, "JTSAN: OtherThread id out of bounds");

    JTSanThreadState::epoch[threadId].set(otherThreadId, epoch);
}

uint32_t JTSanThreadState::getEpoch(size_t threadId, size_t otherThreadId) {
    assert(threadId < JTSanThreadState::size, "JTSAN: Thread id out of bounds");
    assert(otherThreadId < JTSanThreadState::size, "JTSAN: Thread id out of bounds");

    return JTSanThreadState::epoch[threadId].get(otherThreadId);
}

void JTSanThreadState::transferEpoch(size_t from_tid, size_t to_tid) {
    assert(from_tid < JTSanThreadState::size, "JTSAN: Thread id out of bounds");
    assert(to_tid < JTSanThreadState::size, "JTSAN: OtherThread id out of bounds");
    
    JTSanThreadState::epoch[to_tid] = JTSanThreadState::epoch[from_tid];
}


ThreadHistory* JTSanThreadState::getHistory(int threadId) {
    assert(threadId < MAX_THREADS, "JTSAN: Thread id out of bounds");

    return JTSanThreadState::history[threadId];
}