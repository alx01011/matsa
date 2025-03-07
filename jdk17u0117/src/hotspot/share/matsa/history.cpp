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
    parts = (Part*)os::reserve_memory(MAX_PARTS * sizeof(Part));
    bool protect = os::protect_memory((char*)parts, MAX_PARTS * sizeof(Part), os::MEM_PROT_RW);

    assert(parts != nullptr && protect, "MATSA: Failed to allocate history buffer\n");

    for (int i = 0; i < MAX_PARTS; i++) {
        parts[i].events = (Event*)os::reserve_memory(MAX_EVENTS * sizeof(Event));
        protect = os::protect_memory((char*)parts[i].events, MAX_EVENTS * sizeof(Event), os::MEM_PROT_RW);

        assert(parts[i].events != nullptr && protect, "MATSA: Failed to allocate history buffer\n");

        // allocate the real stack
        parts[i].real_stack = (uint64_t)os::reserve_memory(DEFAULT_STACK_SIZE * sizeof(uint64_t));
        parts[i].real_stack_size = 0;
        protect = os::protect_memory((char*)((uint64_t)parts[i].real_stack), DEFAULT_STACK_SIZE * sizeof(uint64_t), os::MEM_PROT_RW);

        assert(protect, "MATSA: Failed to allocate history buffer real stack\n");
    }

    event_idx  = 0;
    part_idx   = 0;
}

History::~History() {
    for (int i = 0; i < MAX_PARTS; i++) {
        os::release_memory((char*)parts[i].events, MAX_EVENTS * sizeof(Event));
    }

    os::release_memory((char*)parts, MAX_PARTS * sizeof(Part));
}

void History::add_event(JavaThread *thread, Method *m, uint16_t bci) {
    uint64_t tid = JavaThread::get_matsa_tid(thread);
    History *h = history[tid];

    if (h->event_idx == MAX_EVENTS - 1) {
        h->event_idx = 0;
        h->part_idx = (h->part_idx + 1) & (MAX_PARTS - 1); // its a power of 2
        // increment epoch
        h->parts[h->part_idx].epoch++;

        MaTSaStack *stack = JavaThread::get_matsa_stack(thread);
        memcpy((uint64_t*)((uint64_t)h->parts[h->part_idx].real_stack), stack->get(), stack->size() * sizeof(uint64_t));
        h->parts[h->part_idx].real_stack_size = stack->size();
    }

    Part *current_part = &h->parts[h->part_idx];
    Event *event = &current_part->events[h->event_idx];

    event->method = (uint64_t)m;
    event->bci = bci;

    h->event_idx++;
}

Part *History::get_part(uint64_t tid, uint64_t idx) {
    return &history[tid]->parts[idx];
}

uint64_t History::get_part_idx(uint64_t tid) {
    return history[tid]->part_idx;
}

uint64_t History::get_event_idx(uint64_t tid) {
    return history[tid]->event_idx;
}

uint64_t History::get_cur_epoch(uint64_t tid) {
    return history[tid]->parts[history[tid]->part_idx].epoch;
}

History *History::get_history(uint64_t tid) {
    return history[tid];
}

void History::clear_history(uint64_t tid) {
    History *h = history[tid];

    for (int i = 0; i < h->part_idx; i++) {
        h->parts[i].epoch = 0;
    }

    h->part_idx = 0;
    h->event_idx = 0;
}