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
    // because index is unsinged and has a width of 9 bits, it will wrap around
    // effectively invalidating the buffer by setting the index to 0
    
    events[index.fetch_add(1, std::memory_order_seq_cst)] = event;
}

uint64_t ThreadHistory::get_event(int i) {
    if (i >= index.load(std::memory_order_seq_cst)) {
        return 0;
    }

    return events[i];
}

uintptr_t Symbolizer::CompressAddr(uintptr_t addr) {
    return addr & ((1ull << COMPRESSED_ADDR_BITS) - 1); // see jtsanDefs
}

uintptr_t Symbolizer::RestoreAddr(uintptr_t addr) {
    return addr | (ShadowMemory::heap_base & ~((1ull << COMPRESSED_ADDR_BITS) - 1));
}

void Symbolizer::Symbolize(Event event, void *addr, int bci, int tid) {
    /*
        Layout of packed event uint64_t:
        | event (2 bits) | addr (48 bits) | bci (14 bits) |
        |  0...1         | 2...49         | 50...63       |

        The only field that may not fit is bci, but it won't cause problems in unpacking
        because it can only occupy the last 14 bits
    */

    uint64_t e = (uint64_t) event | (uint64_t)addr << 2 | (uint64_t)bci << 50;

    ThreadHistory *history = JTSanThreadState::getHistory(tid);
    history->add_event(e);
}

bool Symbolizer::TraceUpToAddress(JTSanEventTrace &trace, void *addr, int tid, ShadowCell &prev) {
    ThreadHistory *history = JTSanThreadState::getHistory(tid);
    bool found = false;

    uint16_t sp = 0;

    for (int i = 0; i < EVENT_BUFFER_SIZE; i++) {
        uint64_t   raw_event = history->get_event(i);
        // type punning is the fastest way to convert the raw event to a JTSanEvent
        // struct fields are in the correct order
        JTSanEvent e = *(JTSanEvent*)&raw_event;

        switch(e.event) {
            case FUNC:
                switch(e.pc) {
                    case 0: // method exit
                        if (sp > 0) {
                            sp--;
                        }
                        break;
                    default: // method entry
                        trace.events[sp++] = e;
                        // in case sp gets out of bounds it wraps around
                        // start overwriting the oldest events
                        // stack trace doesn't have to be complete
                        // the last events are the most important
                        break;
                }
                break;
            case MEM_READ:
            case MEM_WRITE: {
                uintptr_t pc = e.pc;

                if (e.event == (Event)(prev.is_write + 1) && pc == (uintptr_t)addr) {
                    if (sp > 0) {
                        trace.events[sp - 1].bci = e.bci;
                        trace.size = sp;

                        found = true;
                    }

                    return found;
                }
                break;
            }
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