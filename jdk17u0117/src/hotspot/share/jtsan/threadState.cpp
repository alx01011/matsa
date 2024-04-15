#include "threadState.hpp"


#include "runtime/atomic.hpp"
#include "utilities/debug.hpp"
#include "runtime/os.hpp"

JtsanThreadState* JtsanThreadState::instance = nullptr;

JtsanThreadState::JtsanThreadState(void) {
  this->size = MAX_THREADS;

  this->epoch = (uint32_t (*)[MAX_THREADS])os::reserve_memory(this->size * sizeof(uint32_t[MAX_THREADS]));

  if (this->epoch == nullptr) {
    fprintf(stderr, "JTSAN: Failed to allocate thread state memory\n");
    exit(1);
  }

  bool protect = os::protect_memory((char*)this->epoch, this->size * sizeof(uint32_t[MAX_THREADS]), os::MEM_PROT_RW);
  if (!protect) {
    fprintf(stderr, "JTSAN: Failed to protect thread state\n");
    exit(1);
  }
}

JtsanThreadState::~JtsanThreadState(void) {
    this->size = 0;

    for (size_t i = 0; i < this->size; i++) {
        if (this->epoch[i] != nullptr) {
            os::release_memory((char*)this->epoch[i], this->size * sizeof(uint32_t));
        }
    }

    if (this->epoch != nullptr) {
        os::release_memory((char*)this->epoch, this->size * sizeof(uint32_t*));
    }

    JtsanThreadState::instance = nullptr;
}

JtsanThreadState* JtsanThreadState::getInstance(void) {
    if (instance == nullptr) {
        instance = new JtsanThreadState();
    }
    return instance;
}

uint32_t* JtsanThreadState::getThreadState(size_t threadId) {
    JtsanThreadState *state = JtsanThreadState::getInstance();

    return nullptr;
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

    assert(threadId < state->size, "JTSAN: Thread id out of bounds");
    // might be unnecessary
    Atomic::inc(&(state->epoch[threadId][threadId]));
}

void JtsanThreadState::setEpoch(size_t threadId, size_t otherThreadId, uint32_t epoch) {
    JtsanThreadState *state = JtsanThreadState::getInstance();

    assert(threadId < state->size, "JTSAN: Thread id out of bounds");
    assert(otherThreadId < state->size, "JTSAN: OtherThread id out of bounds");

    Atomic::store(&(state->epoch[threadId][otherThreadId]), epoch);
}

uint32_t JtsanThreadState::getEpoch(size_t threadId, size_t otherThreadId) {
    JtsanThreadState *state = JtsanThreadState::getInstance();

    assert(threadId < state->size, "JTSAN: Thread id out of bounds");
    assert(otherThreadId < state->size, "JTSAN: Thread id out of bounds");

    uint32_t res = Atomic::load(&(state->epoch[threadId][otherThreadId]));

    return res;
}

void JtsanThreadState::maxEpoch(size_t threadId, size_t otherThreadId, uint32_t epoch) {
    JtsanThreadState *state = JtsanThreadState::getInstance();

    assert(threadId < state->size, "JTSAN: Thread id out of bounds");
    assert(otherThreadId < state->size, "JTSAN: OtherThread id out of bounds");

    uint32_t current = Atomic::load(&(state->epoch[threadId][otherThreadId]));

    while (epoch > current) {
        if (Atomic::cmpxchg(&(state->epoch[threadId][otherThreadId]), current, epoch) == current) {
            break;
        }
        current = Atomic::load(&(state->epoch[threadId][otherThreadId]));
    }
}