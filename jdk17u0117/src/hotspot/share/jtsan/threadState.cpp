#include "threadState.hpp"


#include "runtime/atomic.hpp"
#include "utilities/debug.hpp"
#include "runtime/os.hpp"

#include <string.h>

JtsanThreadState* JtsanThreadState::instance = nullptr;

JtsanThreadState::JtsanThreadState(void) {
  this->size = MAX_THREADS * sizeof(Vectorclock);

  //this->epoch = (uint32_t (*)[MAX_THREADS])os::attempt_reserve_memory_at((char*)kMetaShadowBeg, this->size * sizeof(uint32_t[MAX_THREADS]));
  this->epoch = (Vectorclock*)os::reserve_memory(this->size);

  if (this->epoch == nullptr) {
    fprintf(stderr, "JTSAN: Failed to allocate thread state memory\n");
    exit(1);
  }

  bool protect = os::protect_memory((char*)this->epoch, this->size, os::MEM_PROT_RW);
  if (!protect) {
    fprintf(stderr, "JTSAN: Failed to protect thread state\n");
    exit(1);
  }

  // increment the epoch of the initial thread
    this->epoch[0].set(0, 1);
}

JtsanThreadState::~JtsanThreadState(void) {
    if (this->epoch != nullptr) {
        os::release_memory((char*)this->epoch, this->size);
    }

    this->epoch = nullptr;
    this->size = 0;

    JtsanThreadState::instance = nullptr;
}

JtsanThreadState* JtsanThreadState::getInstance(void) {
    if (instance == nullptr) {
        instance = new JtsanThreadState();
    }
    return instance;
}

Vectorclock* JtsanThreadState::getThreadState(size_t threadId) {
    JtsanThreadState *state = JtsanThreadState::getInstance();

    assert(threadId < MAX_THREADS, "JTSAN: Thread id out of bounds");

    if (threadId >= MAX_THREADS) {
        fprintf(stderr, "JTSAN: Thread id %lu out of bounds\n", threadId);
    }

    return &(state->epoch[threadId]);
}

void JtsanThreadState::init(void) {
    if (instance == nullptr) {
        instance = new JtsanThreadState();
    }
}

void JtsanThreadState::destroy(void) {
    if (instance != nullptr) {
        delete instance;
        instance = nullptr;
    }
}

void JtsanThreadState::incrementEpoch(size_t threadId) {
    JtsanThreadState *state = JtsanThreadState::getInstance();

    assert(threadId < MAX_THREADS, "JTSAN: Thread id out of bounds");
    
    state->epoch[threadId].set(threadId, state->epoch[threadId].get(threadId) + 1);
}

void JtsanThreadState::setEpoch(size_t threadId, size_t otherThreadId, uint32_t epoch) {
    JtsanThreadState *state = JtsanThreadState::getInstance();

    assert(threadId < MAX_THREADS, "JTSAN: Thread id out of bounds");
    assert(otherThreadId < MAX_THREADS, "JTSAN: OtherThread id out of bounds");

    state->epoch[threadId].set(otherThreadId, epoch);

    //Atomic::store(&(state->epoch[threadId][otherThreadId]), epoch);
}

uint32_t JtsanThreadState::getEpoch(size_t threadId, size_t otherThreadId) {
    JtsanThreadState *state = JtsanThreadState::getInstance();

    assert(threadId < state->size, "JTSAN: Thread id out of bounds");
    assert(otherThreadId < state->size, "JTSAN: Thread id out of bounds");

    //uint32_t res = Atomic::load(&(state->epoch[threadId][otherThreadId]));

    return state->epoch[threadId].get(otherThreadId);
}

void JtsanThreadState::maxEpoch(size_t threadId, size_t otherThreadId, uint32_t epoch) {
    JtsanThreadState *state = JtsanThreadState::getInstance();

    assert(threadId < MAX_THREADS, "JTSAN: Thread id out of bounds");
    assert(otherThreadId < MAX_THREADS, "JTSAN: OtherThread id out of bounds");

    // uint32_t current = Atomic::load(&(state->epoch[threadId][otherThreadId]));

    // while (epoch > current) {
    //     if (Atomic::cmpxchg(&(state->epoch[threadId][otherThreadId]), current, epoch) == current) {
    //         break;
    //     }
    //     current = Atomic::load(&(state->epoch[threadId][otherThreadId]));
    // }
}

void JtsanThreadState::transferEpoch(size_t from_tid, size_t to_tid) {
    JtsanThreadState *state = JtsanThreadState::getInstance();

    assert(from_tid < state->size, "JTSAN: Thread id out of bounds");
    assert(to_tid < state->size, "JTSAN: OtherThread id out of bounds");
    
    state->epoch[to_tid] = state->epoch[from_tid];


    if (to_tid == 0 && from_tid == 5) {
        fprintf(stderr, "JTSAN: Transferred epoch from %lu to %lu\n", from_tid, to_tid);
    }
}

void JtsanThreadState::clearEpoch(size_t threadId) {
    JtsanThreadState *state = JtsanThreadState::getInstance();

    assert(threadId < state->size, "JTSAN: Thread id out of bounds");
}