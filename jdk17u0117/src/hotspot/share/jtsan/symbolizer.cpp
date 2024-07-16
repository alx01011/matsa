#include "symbolizer.hpp"
#include "jtsanGlobals.hpp"
#include "threadState.hpp"

#include "memory/allocation.hpp"
#include "runtime/os.hpp"

#include <cstring>

ThreadHistory::ThreadHistory() {
    index = 0;
    lock = new Mutex(Mutex::access, "JTSanThreadHistory::_history_lock");

    events      = (uint64_t*)os::reserve_memory(EVENT_BUFFER_SIZE * sizeof(uint64_t));
    event_epoch = (uint64_t*)os::reserve_memory(EVENT_BUFFER_SIZE * sizeof(uint64_t));

    if (!events || !event_epoch) {
        fatal("JTSan Symbolizer: Failed to mmap");
    }

    bool protect = os::protect_memory((char*)events, EVENT_BUFFER_SIZE * sizeof(uint64_t), os::MEM_PROT_RW)
                   && os::protect_memory((char*)event_epoch, EVENT_BUFFER_SIZE * sizeof(uint64_t), os::MEM_PROT_RW);

    if (!protect) {
        fatal("JTSan Symbolizer: Failed to protect memory");
    }
}

void ThreadHistory::add_event(uint64_t event, uint32_t epoch) {
    // if the buffer gets full, there is a small chance that we will report the wrong trace
    // might happen if slots before the access get filled with method entry/exit events
    // if it gets filled we invalidate
    // because index is unsinged it will wrap around
    // effectively invalidating the buffer by setting the index to 0

    uint16_t i = index.fetch_add(1, std::memory_order_relaxed);

    events[i]      = event;
    event_epoch[i] = epoch;

}

uint64_t ThreadHistory::get_event(int i) {
    if (i >= index.load(std::memory_order_relaxed)) {
        return 0;
    }

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
    history->add_event(e, epoch);
}

bool Symbolizer::TraceUpToAddress(JTSanEventTrace &trace, void *addr, int tid, ShadowCell &prev) {
    ThreadHistory *history = JTSanThreadState::getHistory(tid);

    uint16_t sp   = 0;

    int last = 0;

    // find last occurrence of the address
    // we start traversing from the end of the buffer
    // last access is more likely to be closer to the end
    for (int i = history->index.load(std::memory_order_relaxed) - 1; i >= 0; i--) {
        uint64_t raw_event = history->get_event(i);
        JTSanEvent e = *(JTSanEvent*)&raw_event;

        switch (e.event) {
            case MEM_READ:
            case MEM_WRITE: {
                if ((Event)(prev.is_write + 1) == e.event && (void*)e.pc == addr) {
                    uint32_t epoch = history->get_epoch(i);
                    if (epoch != prev.epoch) {
                        continue; // epoch mismatch probably a previous access
                    }

                    last = i;
                    i  = 0; // break out of the loop
                }
                break;
            }
            case INVALID:
            default:
                break;
        }
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
                    uint32_t epoch = history->get_epoch(i);
                    if (epoch != prev.epoch) {
                        continue; // epoch mismatch probably a previous access
                    }

                    if (sp > 0) {
                        trace.events[sp - 1].bci = e.bci;
                        trace.size = sp;

                        return true;
                    }

                    return false;
                }
                break;
            }
            case INVALID:
            default:
                return false;
        }
    
    }

    return last != 0;
}

void Symbolizer::ClearThreadHistory(int tid) {
    ThreadHistory *history = JTSanThreadState::getHistory(tid);
    history->clear();
}