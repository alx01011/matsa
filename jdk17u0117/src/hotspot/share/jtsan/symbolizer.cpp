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
    JTSanScopedLock(this->lock);
    events[index++] = event;
}

uint64_t ThreadHistory::get_event(int i) {
    // if (i >= index.load(std::memory_order_seq_cst)) {
    //     return 0;
    // }
    // must hold lock
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

   // this actually produces better assembly than initializing a struct
    uint64_t e = (uint64_t) event | (uint64_t)addr << 2 | (uint64_t)bci << 50;

    ThreadHistory *history = JTSanThreadState::getHistory(tid);
    history->add_event(e);
}

bool Symbolizer::TraceUpToAddress(JTSanEventTrace &trace, void *addr, int tid, ShadowCell &prev) {
    ThreadHistory *history = JTSanThreadState::getHistory(tid);

    JTSanScopedLock(history->lock);

    bool found = false;

    uint16_t sp = 0;

    int last = -1;

    for (int i = 0; i < EVENT_BUFFER_SIZE; i++) {
        uint64_t   raw_event = history->get_event(i);
        // type punning is the fastest way to convert the raw event to a JTSanEvent
        // struct fields are in the correct order
        JTSanEvent e = *(JTSanEvent*)&raw_event;

        if (e.event == INVALID) {
            break;
        }

        if ((void*)e.pc == addr) {
            last = i;
        }

        // switch(e.event) {
        //     case FUNC:
        //         switch(e.pc) {
        //             case 0: // method exit
        //                 if (sp > 0) {
        //                     sp--;
        //                 }
        //                 break;
        //             default: // method entry
        //                 trace.events[sp++] = e;
        //                 // in case sp gets out of bounds it wraps around
        //                 // start overwriting the oldest events
        //                 // stack trace doesn't have to be complete
        //                 // the last events are the most important
        //                 break;
        //         }
        //         break;
        //     case MEM_READ:
        //     case MEM_WRITE: {
        //         if (e.event == (Event)(prev.is_write + 1) && (void*)e.pc == addr) {
        //             if (sp > 0) {
        //                 trace.events[sp - 1].bci = e.bci;
        //                 trace.size = sp;

        //                 found = true;
        //             }

        //             return found;
        //         }
        //         break;
        //     }
        //     case INVALID:
        //     default:
        //         return false;
        // }
    }

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
                if (i == last - 1) {
                    if (sp > 0) {
                        trace.events[sp - 1].bci = e.bci;
                        trace.size = sp;

                        found = true;
                    }

                    return found;
                }
                break;
            }
            // should never reach here
            case INVALID:
            default:
                return false;
        }
    
    }

    return last != -1;
}

void Symbolizer::ClearThreadHistory(int tid) {
    ThreadHistory *history = JTSanThreadState::getHistory(tid);
    history->clear();
}