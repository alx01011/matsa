#ifndef THREADSTATE_HPP
#define THREADSTATE_HPP

#include "memory/allocation.hpp"
#include "runtime/mutex.hpp"

#include "vectorclock.hpp"

#include <cstddef>
#include <cstdint>

class MaTSaThreadState : AllStatic {
    private:        
        /*
            Each thread has an array of epochs.
            Epochs can change after a synchronization event
        */
        static Vectorclock *epoch;
        static size_t   size;

    public:
        static void init(void);
        static void destroy(void);

        // returns the thread state of thread threadId
        static Vectorclock* getThreadState(size_t threadId);

        static void     setEpoch(size_t threadId, size_t otherThreadId, uint32_t epoch);

        // increments the epoch of the thread with id threadId
        static void     incrementEpoch(size_t threadId);
        // gets the epoch of the thread with id otherThreadId from the thread with id threadId epoch[threadId][otherThreadId]
        static uint32_t getEpoch(size_t threadId, size_t otherThreadId);

        static void     transferEpoch(size_t from_tid, size_t to_tid);
};

#endif