#ifndef SYMBOLIZER_HPP
#define SYMBOLIZER_HPP

#define EVENT_BUFFER_SIZE (1 << 8) // 65536 (2^16)
#define EVENT_BUFFER_WIDTH (8)

#include <cstdint>

#include "runtime/mutex.hpp"
#include "runtime/thread.hpp"
#include "runtime/atomic.hpp"
#include "memory/allocation.hpp"

// we don't need to keep track the if the access was a read or write
// we already have that information in the shadow cell
enum Event {
    ACCESS,
    METHOD_ENTRY,
    METHOD_EXIT
};

class JTSanEvent {
    public:
        Event     event : 2; // 3 events
        int       bci   : 14; // this limits the number of bytecodes to 16384
        uintptr_t pc    : 48; // 48 bits on most systems
};

class JTSanEventTrace {
    public:
        JTSanEvent events[EVENT_BUFFER_SIZE];
        int        size;
};

// for each thread we keep a cyclic buffer of the last 256 events
class ThreadHistory : public CHeapObj<mtInternal>{
    private:
        JTSanEvent events[EVENT_BUFFER_SIZE];
        uint8_t   index; // 256 events at most
        // instead of locking, is it faster to do an atomic increment on index and just load whatever is on events?
        // a single event is 8 bytes and the memory is dword aligned, so we are safe
        // maybe 8 events could share a cache line too?
        Mutex     *lock;
    public:
        ThreadHistory();

        void add_event(JTSanEvent event);
        JTSanEvent get_event(int i);

        void clear(void) {
            Atomic::store(&index, 0);
            // lock->lock();
            // index = 0;
            // lock->unlock();
        }
};

namespace Symbolizer {
    void Symbolize       (Event event, void *addr, int bci, int tid);
    bool TraceUpToAddress(JTSanEventTrace &trace, void *addr, int tid);

    void ClearThreadHistory(int tid);
};

#endif