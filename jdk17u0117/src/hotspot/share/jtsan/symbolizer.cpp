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

void ThreadHistory:: add_event(uint64_t event) {
    // if the buffer gets full, there is a small chance that we will report the wrong trace
    // might happen if slots before the access get filled with method entry/exit events
    // if it gets filled we invalidate
    // because index is unsinged it will wrap around
    // effectively invalidating the buffer by setting the index to 0
    events[index++] = event;
}

uint64_t ThreadHistory::get_event(int i) {    
    return events[i];
}

uint64_t ThreadHistory::get_epoch(int i) {
    return event_epoch[i];
}

uintptr_t Symbolizer::CompressAddr(uintptr_t addr) {
    return addr & ((1ull << COMPRESSED_ADDR_BITS) - 1); // see jtsanDefs
}

uintptr_t Symbolizer::RestoreAddr(uintptr_t addr) {
    return addr | (ShadowMemory::heap_base & ~((1ull << COMPRESSED_ADDR_BITS) - 1));
}

void Symbolizer::Symbolize(Event event, void *addr, int bci, int tid, uint32_t epoch) {
    /*
        Layout of packed event uint64_t:
        | event (2 bits) | addr (48 bits) | bci (14 bits) |
        |  0...1         | 2...49         | 50...63       |

        The only field that may not fit is bci, but it won't cause problems in unpacking
        because it can only occupy the last 14 bits
    */

   // this actually produces better assembly than initializing a struct
    uint64_t e = (uint64_t) event | (uint64_t)addr << 2 | (uint64_t)bci << 50;

    ThreadHistory *history = JTSanThreadState::getHistory(tid);
    history->add_event(e);
}

bool Symbolizer::TraceUpToAddress(JTSanEventTrace &trace, void *addr, int tid, ShadowCell &prev) {
    ThreadHistory *history = JTSanThreadState::getHistory(tid);

    uint16_t sp   = 0;
    uint16_t last = history->index.load(std::memory_order_seq_cst);

    for (int i = 0; i < last; i++) {
        uint64_t raw_event = history->get_event(i);
        JTSanEvent e = *(JTSanEvent*)&raw_event;

        switch (e.event) {
            case FUNC:
                switch(e.pc) {
                    case 0: // method exit
                        if (sp > 0) {
                            sp--;
                        }
                        break;
                    default: // method entry
                        trace.events[sp++] = e;
                        break;
                }
                break;
            case MEM_READ:
            case MEM_WRITE: {
                if ((Event)(prev.is_write + 1) == e.event && (void*)e.pc == addr 
                        && history->get_epoch(i) == prev.epoch) {
                    if (sp > 0) {
                        trace.events[sp - 1].bci = e.bci;
                        trace.size = sp;

                        return true;
                    }

                    return false;
                }
                break;
            }
            // should never reach here
            case INVALID:
            default:
                return false;
        }
    
    }

    return false;
}

void Symbolizer::ClearThreadHistory(int tid) {
    ThreadHistory *history = JTSanThreadState::getHistory(tid);
    history->clear();
}