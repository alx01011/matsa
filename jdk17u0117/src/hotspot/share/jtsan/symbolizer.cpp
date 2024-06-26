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

void ThreadHistory::add_event(JTSanEvent event) {
    JTSanScopedLock scopedLock(lock);

    // if the buffer gets full, there is a small chance that we will report the wrong trace
    // might happen if slots before the access get filled with method entry/exit events
    // if it gets filled we invalidate
    // because index is unsinged and has a width of 9 bits, it will wrap around
    // effectively invalidating the buffer by setting the index to 0

    events[index++] = event;
}

JTSanEvent ThreadHistory::get_event(int i) {
    if (i < 0 || i >= EVENT_BUFFER_SIZE || i >= index) {
        JTSanEvent e;
        e.event = ACCESS;
        e.bci = 0;
        e.pc = 0;
        return e;
    }

    JTSanScopedLock scopedLock(lock);

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

bool Symbolizer::TraceUpToAddress(JTSanEventTrace &trace, void *addr, int tid) {
    ThreadHistory *history = JtsanThreadState::getInstance()->getHistory(tid);
    bool found = false;

    JTSanEvent func_stack[EVENT_BUFFER_SIZE];
    int sp = 0;

    int i;
    for (i = 0; i < EVENT_BUFFER_SIZE; i++) {
        JTSanEvent e = history->get_event(i);
        if (e.pc == 0) {
            break; // no more events
        }

        if (e.pc == (uintptr_t)addr) {
            found = true; // we don't want to include the access event

            // change the bci of the previous event to the bci of the access
            // this will give the actual line of the access instead of the line of the method
            if (sp > 0) {
                func_stack[sp - 1].bci = e.bci;
            }

            break;
        }

        if (e.event == METHOD_ENTRY) {
            func_stack[sp++] = e;
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
    for (int j = 0; j < sp; j++) {
        // store in reverse order
        trace.events[j] = func_stack[sp - j - 1];
    }


    return found;
}

void Symbolizer::ClearThreadHistory(int tid) {
    ThreadHistory *history = JtsanThreadState::getInstance()->getHistory(tid);
    history->clear();
}