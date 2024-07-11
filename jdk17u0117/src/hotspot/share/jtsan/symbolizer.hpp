#ifndef SYMBOLIZER_HPP
#define SYMBOLIZER_HPP

#define EVENT_BUFFER_WIDTH (30)
#define EVENT_BUFFER_SIZE (1 << EVENT_BUFFER_WIDTH)


#include <cstdint>
#include <atomic>

#include "jtsanDefs.hpp"
#include "jtsanGlobals.hpp"
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
        JTSanEvent events[EVENT_BUFFER_SIZE];
        int        size;
};

// for each thread we keep a cyclic buffer of the last 256 events
class ThreadHistory : public CHeapObj<mtInternal>{
    private:
        uint64_t events[EVENT_BUFFER_SIZE];
        //uint8_t   index; // 256 events at most
    public:
        ThreadHistory();

        // lock because we try to find the last event
        Mutex    *lock;
        uint64_t index : EVENT_BUFFER_WIDTH;

        // std::atomic<uint16_t> index;

        void add_event(uint64_t event);
        uint64_t get_event(int i);

        void clear(void) {
            JTSanScopedLock l(lock);
            index = 0;
            //index.store(0, std::memory_order_seq_cst);
            // lock->lock();
            // index = 0;
            // lock->unlock();
        }
};

namespace Symbolizer {
    uintptr_t CompressAddr(uintptr_t addr);
    uintptr_t RestoreAddr(uintptr_t addr);

    void Symbolize       (Event event, void *addr, int bci, int tid);
    bool TraceUpToAddress(JTSanEventTrace &trace, void *addr, int tid, ShadowCell &prev);

    void ClearThreadHistory(int tid);
};

#endif