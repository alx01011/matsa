#include "symbolizer.hpp"
#include "jtsanGlobals.hpp"
#include "threadState.hpp"

#include "memory/allocation.hpp"
#include "runtime/os.hpp"

#include <cstring>

ThreadHistory::ThreadHistory() {
    index = 0;
    lock = new Mutex(Mutex::access, "JTSanThreadHistory::_history_lock");

    events            = (uint64_t*)os::reserve_memory(EVENT_BUFFER_SIZE * sizeof(uint64_t));
    event_shadow_addr = (void**)os::reserve_memory(EVENT_BUFFER_SIZE * sizeof(void**));

    if (!events || !event_shadow_addr) {
        fatal("JTSan Symbolizer: Failed to mmap");
    }

    bool protect = os::protect_memory((char*)events, EVENT_BUFFER_SIZE * sizeof(uint64_t), os::MEM_PROT_RW)
                   && os::protect_memory((char*)event_shadow_addr, EVENT_BUFFER_SIZE * sizeof(void**), os::MEM_PROT_RW);

    if (!protect) {
        fatal("JTSan Symbolizer: Failed to protect memory");
    }
}

struct EventIndex {
    uint64_t idx : EVENT_BUFFER_WIDTH;
};

void ThreadHistory::add_event(uint64_t event, void* store_addr) {
    // if the buffer gets full, there is a small chance that we will report the wrong trace
    // might happen if slots before the access get filled with method entry/exit events
    // if it gets filled we invalidate
    // because index is unsinged it will wrap around
    // effectively invalidating the buffer by setting the index to 0

    EventIndex index = { .idx = (this->index++)};

    events[index.idx]            = event;
    event_shadow_addr[index.idx] = store_addr;

}

uint64_t ThreadHistory::get_event(uint32_t i) {
    EventIndex index = { .idx = this->index };

    if (i >= index.idx) {
        return 0;
    }

    return events[i];
}

void *ThreadHistory::get_old_shadow(uint32_t i) {
    return event_shadow_addr[i];
}

uintptr_t Symbolizer::CompressAddr(uintptr_t addr) {
    return addr & ((1ull << COMPRESSED_ADDR_BITS) - 1); // see jtsanDefs
}

uintptr_t Symbolizer::RestoreAddr(uintptr_t addr) {
    return addr | (ShadowMemory::heap_base & ~((1ull << COMPRESSED_ADDR_BITS) - 1));
}

void Symbolizer::Symbolize(Event event, void *addr, int bci, int tid, void *store_addr) {
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
    history->add_event(e, store_addr);
}

bool Symbolizer::TraceUpToAddress(JTSanEventTrace &trace, void *addr, int tid, ShadowCell &prev, void *prev_shadow_addr) {
    ThreadHistory *history = JTSanThreadState::getHistory(tid);

    uint16_t sp = 0;

    int last = 0;

    EventIndex index = { .idx = history->index - 1 };

    // find last occurrence of the address
    // we start traversing from the end of the buffer
    // last access is more likely to be closer to the end
    for (int i = index.idx; i >= 0; i--) {
        uint64_t raw_event = history->get_event(i);
        JTSanEvent e = *(JTSanEvent*)&raw_event;

        switch (e.event) {
            case MEM_READ:
            case MEM_WRITE: {
                if ((Event)(prev.is_write + 1) == e.event && (void*)((uintptr_t)e.pc) == addr) {
                    void* shadow_old = history->get_old_shadow(i);
                    if (shadow_old != prev_shadow_addr) {
                        continue; // shadow address mismatch, probably a different access
                    }

                    last = i;
                    i    = 0; // break
                }
                break;
            }
            case INVALID:
                return false;
            default:
                break;
        }
    }

    uint64_t raw_event;
    JTSanEvent e;

    // traverse up to last but not include last
    for (int i = 0; i < last; i++) {
        raw_event = history->get_event(i);
        e         = *(JTSanEvent*)&raw_event;

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
            default:
                break;
        }
    }

    if (sp > 0) {
        // include the last event
        raw_event = history->get_event(last);

        if (!raw_event) {
            return false;
        }

        e = *(JTSanEvent*)&raw_event;

        trace.events[sp - 1].bci = e.bci;
        trace.size = sp;

        return true;
    }

    return false;
}

void Symbolizer::ClearThreadHistory(int tid) {
    ThreadHistory *history = JTSanThreadState::getHistory(tid);
    history->clear();
}