#ifndef SYMBOLIZER_HPP
#define SYMBOLIZER_HPP

#define EVENT_BUFFER_WIDTH (16)
#define EVENT_BUFFER_SIZE  (1 << EVENT_BUFFER_WIDTH) // 65k

#define MAX_TRACE_SIZE     (1 << 16) // 65k


#include <cstdint>
#include <atomic>

#include "jtsanDefs.hpp"
#include "shadow.hpp"

#include "runtime/mutex.hpp"
#include "runtime/thread.hpp"
#include "runtime/atomic.hpp"
#include "memory/allocation.hpp"

// we don't need to keep track the if the access was a read or write
// we already have that information in the shadow cell
enum Event {
    INVALID   = 0,
    MEM_READ  = 1,
    MEM_WRITE = 2,
    FUNC      = 3
};

class JTSanEvent {
    public:
        Event     event : 2; // 4 events
        uintptr_t pc    : 48; // can't easily compress this
        int       bci   : 64 - 48 - 2;
};

class JTSanEventTrace {
    public:
        JTSanEvent events[MAX_TRACE_SIZE];
        int      size;
};

// for each thread we keep a cyclic buffer of the last 256 events
class ThreadHistory : public CHeapObj<mtInternal>{
    private:
        uint64_t *events;
        // can we do better in terms of memory?
        void    **event_shadow_addr;
        //uint8_t   index; // 256 events at most
        // instead of locking, is it faster to do an atomic increment on index and just load whatever is on events?
        // a single event is 8 bytes and the memory is dword aligned, so we are safe
        // maybe 8 events could share a cache line too?
    public:
        ThreadHistory();
        uint64_t index;

        void add_event(uint64_t event, void *shadow_addr = nullptr);

        uint64_t get_event(uint32_t i);
        void *get_old_shadow(uint32_t i);

        void clear(void) {
            index = 0;
            // lock->lock();
            // index = 0;
            // lock->unlock();
        }
};

namespace Symbolizer {
    uintptr_t CompressAddr(uintptr_t addr);
    uintptr_t RestoreAddr(uintptr_t addr);

    void Symbolize       (Event event, void *addr, int bci, int tid, void *shadow_addr = nullptr);
    bool TraceUpToAddress(JTSanEventTrace &trace, void *addr, int tid, ShadowCell &prev, void *prev_shadow_addr);

    void ClearThreadHistory(int tid);
};

#endif