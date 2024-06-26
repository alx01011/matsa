#ifndef SYMBOLIZER_HPP
#define SYMBOLIZER_HPP

#define EVENT_BUFFER_SIZE (256)

#include <cstdint>

#include "runtime/mutex.hpp"
#include "runtime/thread.hpp"
#include "memory/allocation.hpp"

#include "stacktrace.hpp"

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
        JTSanEvent events[256];
        int        index;
        Mutex     *lock;
    public:
        ThreadHistory();

        void add_event(JTSanEvent event);
        JTSanEvent get_event(int i);

        void clear(void) {
            lock->lock();
            index = 0;
            lock->unlock();
        }
};

namespace Symbolizer {
    void Symbolize       (Event event, void *addr, int bci, int tid);
    bool TraceUpToAddress(JTSanEventTrace &trace, void *addr, int tid);

    void ClearThreadHistory(int tid);
};

#endif