#include "symbolizer.hpp"
#include "jtsanGlobals.hpp"
#include "threadState.hpp"

#include "memory/allocation.hpp"

#include <cstring>

ThreadHistory::ThreadHistory() {
    index = 0;
    lock = new Mutex(Mutex::access, "JTSanThreadHistory::_history_lock");

    memset(events, 0, sizeof(JTSanEvent) * EVENT_BUFFER_SIZE);
}

void ThreadHistory::add_event(JTSanEvent &event) {
    // JTSanScopedLock scopedLock(lock);

    // if the buffer gets full, there is a small chance that we will report the wrong trace
    // might happen if slots before the access get filled with method entry/exit events
    // if it gets filled we invalidate
    // because index is unsinged and has a width of 9 bits, it will wrap around
    // effectively invalidating the buffer by setting the index to 0
    
    events[index.fetch_add(1, std::memory_order_seq_cst)] = event;
}

JTSanEvent ThreadHistory::get_event(int i) {
    // JTSanScopedLock scopedLock(lock);

    return events[i];
}

void Symbolizer::Symbolize(Event event, void *addr, int bci, int tid) {
    JTSanEvent e;

    e.event = event;
    e.bci   = bci;
    e.pc    = (uintptr_t)addr;

    ThreadHistory *history = JtsanThreadState::getInstance()->getHistory(tid);
    history->add_event(e);
}

// TODO: For faster lookups we could use a hash table and hash on the address
bool Symbolizer::TraceUpToAddress(JTSanEventTrace &trace, void *addr, int tid) {
    ThreadHistory *history = JtsanThreadState::getInstance()->getHistory(tid);
    bool found = false;

    int sp = 0;
    int i;

    for (i = 0; i < EVENT_BUFFER_SIZE; i++) {
        JTSanEvent e = history->get_event(i);
        if (e.pc == 0) {
            break; // no more events
        }

        if (e.pc == (uintptr_t)addr) {
            // change the bci of the previous event to the bci of the access
            // this will give the actual line of the access instead of the line of the method
            if (found = sp > 0) { // if only one frame, then we don't really have anything useful to report, mark as not found
                trace.events[sp - 1].bci = e.bci;
            }

            break;
        }

        if (e.event == METHOD_ENTRY) {
            trace.events[sp++] = e;
        } else if (e.event == METHOD_EXIT) {
            if (sp > 0) {
                sp--;
            }
        } else {
            // ignore access events
            continue;
        }
    }
    trace.size = sp;

    return found;
}

void Symbolizer::ClearThreadHistory(int tid) {
    ThreadHistory *history = JtsanThreadState::getInstance()->getHistory(tid);
    history->clear();
}