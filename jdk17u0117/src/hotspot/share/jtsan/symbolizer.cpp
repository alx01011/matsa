#include "symbolizer.hpp"
#include "jtsanGlobals.hpp"
#include "threadState.hpp"

#include "memory/allocation.hpp"

ThreadHistory::ThreadHistory() {
    index = 0;
    lock = new Mutex(Mutex::access, "JTSanThreadHistory::_history_lock");
}

void ThreadHistory::add_event(JTSanEvent event) {
    JTSanScopedLock scopedLock(lock);

    events[index] = event;
    index = (index + 1) % EVENT_BUFFER_SIZE;
}

JTSanEvent ThreadHistory::get_event(int i) {
    if (i < 0 || i >= EVENT_BUFFER_SIZE) {
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

    int i;
    for (i = 0; i < EVENT_BUFFER_SIZE; i++) {
        JTSanEvent e = history->get_event(i);
        if (e.pc == 0) {
            break; // no more events
        }

        if (e.pc == (uintptr_t)addr) {
            found = true; // we don't want to include the access event
            break;
        }

        trace.events[i] = e;
    }
    trace.size = i;


    return found;
}

void Symbolizer::ClearThreadHistory(int tid) {
    ThreadHistory *history = JtsanThreadState::getInstance()->getThreadState(tid)->getHistory();
    history->clear();
}