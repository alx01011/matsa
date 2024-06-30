#include "symbolizer.hpp"
#include "jtsanGlobals.hpp"
#include "threadState.hpp"
#include "shadow.hpp"

#include "memory/allocation.hpp"

#include <cstring>

ThreadHistory::ThreadHistory() {
    index = 0;
    lock = new Mutex(Mutex::access, "JTSanThreadHistory::_history_lock");

    memset(events, 0, sizeof(JTSanEvent) * EVENT_BUFFER_SIZE);
}

void ThreadHistory::add_event(JTSanEvent &event) {
    // if the buffer gets full, there is a small chance that we will report the wrong trace
    // might happen if slots before the access get filled with method entry/exit events
    // if it gets filled we invalidate
    // because index is unsinged and has a width of 9 bits, it will wrap around
    // effectively invalidating the buffer by setting the index to 0
    
    events[index.fetch_add(1, std::memory_order_seq_cst)] = event;
}

JTSanEvent ThreadHistory::get_event(int i) {
    if (i >= index.load(std::memory_order_seq_cst)) {
        return {INVALID, 0, 0};
    }

    return events[i];
}

uintptr_t Symbolizer::CompressAddr(uintptr_t addr) {
    return addr & ((1ull << COMPRESSED_ADDR_BITS) - 1);
}

uintptr_t Symbolizer::RestoreAddr(uintptr_t addr) {
    return addr | (ShadowMemory::heap_base & ~((1ull << COMPRESSED_ADDR_BITS) - 1));
}

void Symbolizer::Symbolize(Event event, void *addr, int bci, int tid) {
    JTSanEvent e = {event, bci, (uintptr_t)addr};

    ThreadHistory *history = JtsanThreadState::getInstance()->getHistory(tid);
    history->add_event(e);
}

bool Symbolizer::TraceUpToAddress(JTSanEventTrace &trace, void *addr, int tid) {
    ThreadHistory *history = JtsanThreadState::getInstance()->getHistory(tid);
    bool found = false;

    int sp = 0;
    int i;

    for (i = 0; i < EVENT_BUFFER_SIZE; i++) {
        JTSanEvent e = history->get_event(i);

        switch(e.event) {
            case METHOD_ENTRY:
                trace.events[sp++] = e;
                break;
            case METHOD_EXIT:
                if (sp > 0) {
                    sp--;
                }
                break;
            case ACCESS:
                uintptr_t raw_addres = Symbolizer::RestoreAddr(e.pc);
                if (raw_addres == (uintptr_t)addr) {
                    if (found = sp > 0) {
                        trace.events[sp - 1].bci = e.bci;
                    }
                    goto FOUND;
                }
                break;
            case INVALID:
            default:
                return false;
        }
    }

FOUND:
    trace.size = sp;

    return found;
}

void Symbolizer::ClearThreadHistory(int tid) {
    ThreadHistory *history = JtsanThreadState::getInstance()->getHistory(tid);
    history->clear();
}