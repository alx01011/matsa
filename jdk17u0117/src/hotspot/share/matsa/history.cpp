#include "history.hpp"
#include "matsaStack.hpp"
#include "matsaDefs.hpp"

#include <cstring>
#include "runtime/os.hpp"

History** History::history = nullptr;

void History::init_history(void) {
    assert(history == nullptr, "MATSA: History already initialized\n");
    history = (History**)os::reserve_memory(MAX_THREADS * sizeof(History*));
    bool protect = os::protect_memory((char*)history, MAX_THREADS * sizeof(History*), os::MEM_PROT_RW);
    assert(history != nullptr && protect, "MATSA: Failed to allocate history buffer\n");

    for (int i = 0; i < MAX_THREADS; i++) {
        history[i] = new History();
    }
}

History::History() {
    buffer = (EventBuffer*)os::reserve_memory(MAX_BUFFERS * sizeof(EventBuffer));
    bool protect = os::protect_memory((char*)buffer, MAX_BUFFERS * sizeof(EventBuffer), os::MEM_PROT_RW);

    assert(buffer != nullptr && protect, "MATSA: Failed to allocate history buffer\n");

    for (int i = 0; i < MAX_BUFFERS; i++) {
        buffer[i].events = (EventBuffer::Event*)os::reserve_memory(MAX_EVENTS * sizeof(EventBuffer::Event));
        protect = os::protect_memory((char*)buffer[i].events, MAX_EVENTS * sizeof(EventBuffer::Event), os::MEM_PROT_RW);

        assert(buffer[i].events != nullptr && protect, "MATSA: Failed to allocate history buffer\n");

        // allocate the real stack
        buffer[i].real_stack = (uint64_t*)os::reserve_memory(DEFAULT_STACK_SIZE * sizeof(uint64_t));
        buffer[i].real_stack_size = 0;
        protect = os::protect_memory((char*)buffer[i].real_stack, DEFAULT_STACK_SIZE * sizeof(uint64_t), os::MEM_PROT_RW);

        assert(buffer[i].real_stack != nullptr && protect, "MATSA: Failed to allocate history buffer real stack\n");
    }

    event_idx  = 0;
    buffer_idx = 0;
}

History::~History() {
    for (int i = 0; i < MAX_BUFFERS; i++) {
        os::release_memory((char*)buffer[i].events, MAX_EVENTS * sizeof(EventBuffer::Event));
    }

    os::release_memory((char*)buffer, MAX_BUFFERS * sizeof(EventBuffer));
}

void History::add_event(JavaThread *thread, Method *m, uint16_t bci) {
    uint64_t tid = JavaThread::get_matsa_tid(thread);
    History *h = history[tid];

    if (h->event_idx == MAX_EVENTS - 1) {
        h->buffer_idx++;
        // increment epoch
        h->buffer[h->buffer_idx].epoch++;
        h->event_idx = 0;

        MaTSaStack *stack = JavaThread::get_matsa_stack(thread);
        memcpy(h->buffer[h->buffer_idx].real_stack, stack->get(), stack->size() * sizeof(uint64_t));
        h->buffer[h->buffer_idx].real_stack_size = stack->size();
    }

    EventBuffer *current_buffer = &h->buffer[h->buffer_idx];
    EventBuffer::Event *event = &current_buffer->events[h->event_idx];

    event->method = m;
    event->bci = bci;

    h->event_idx++;
}

EventBuffer *History::get_buffer(uint64_t tid, uint64_t idx) {
    assert(idx < MAX_BUFFERS, "MATSA: Invalid buffer index\n");
    return &history[tid]->buffer[idx];
}

uint64_t History::get_ring_idx(uint64_t tid) {
    return history[tid]->buffer_idx;
}

uint64_t History::get_event_idx(uint64_t tid) {
    return history[tid]->event_idx;
}

uint64_t History::get_cur_epoch(uint64_t tid) {
    return history[tid]->buffer[history[tid]->buffer_idx].epoch;
}

History *History::get_history(uint64_t tid) {
    return history[tid];
}

void History::clear_history(uint64_t tid) {
    History *h = history[tid];

    for (int i = 0; i < h->buffer_idx; i++) {
        h->buffer[i].epoch = 0;
    }

    h->buffer_idx = 0;
    h->event_idx = 0;
}