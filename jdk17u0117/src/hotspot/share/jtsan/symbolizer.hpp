#ifndef SYMBOLIZER_HPP
#define SYMBOLIZER_HPP

#define EVENT_BUFFER_SIZE (1 << 16) // 65536
#define EVENT_BUFFER_WIDTH (16)

#include <cstdint>
#include <atomic>

#include "jtsanDefs.hpp"

#include "runtime/mutex.hpp"
#include "runtime/thread.hpp"
#include "runtime/atomic.hpp"
#include "memory/allocation.hpp"

// we don't need to keep track the if the access was a read or write
// we already have that information in the shadow cell
enum Event {
    INVALID,
    ACCESS,
    METHOD_ENTRY,
    METHOD_EXIT
};

class JTSanEvent {
    public:
        Event     event : 2; // 3 events
        int       bci   : 64 - COMPRESSED_ADDR_BITS - 2;
        uintptr_t pc    : COMPRESSED_ADDR_BITS; // see jtsanDefs
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
        // are 65k events too many?
        std::atomic<uint16_t> index;
        //uint8_t   index; // 256 events at most
        // instead of locking, is it faster to do an atomic increment on index and just load whatever is on events?
        // a single event is 8 bytes and the memory is dword aligned, so we are safe
        // maybe 8 events could share a cache line too?
        Mutex     *lock;
    public:
        ThreadHistory();

        void add_event(JTSanEvent &event);
        JTSanEvent get_event(int i);

        void clear(void) {
            index.store(0, std::memory_order_seq_cst);
            // lock->lock();
            // index = 0;
            // lock->unlock();
        }
};

namespace Symbolizer {
    uintptr_t CompressAddr(uintptr_t addr);
    uintptr_t RestoreAddr(uintptr_t addr);

    void Symbolize       (Event event, void *addr, int bci, int tid);
    bool TraceUpToAddress(JTSanEventTrace &trace, void *addr, int tid);

    void ClearThreadHistory(int tid);
};

#endif