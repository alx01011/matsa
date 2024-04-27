#ifndef THREADSTATE_HPP
#define THREADSTATE_HPP

#include "memory/allocation.hpp"

#include "vectorclock.hpp"

#include <cstddef>
#include <cstdint>

#define MAX_THREADS (256)

class JtsanThreadState : public CHeapObj<mtInternal> {
    private:
        static JtsanThreadState* instance;
        
        /*
            Each thread has an array of epochs.
            It needs to be of MAX_THREADS x MAX_THREADS because each thread has to know the epochs of other threads aswell.
            Epochs can change after a synchronization event, or a memory access
        */

        Vectorclock *epoch;
        size_t   size;

        JtsanThreadState(void);
        ~JtsanThreadState(void);

    public:
        static JtsanThreadState* getInstance(void);

        static void init(void);
        static void destroy(void);

        // returns the thread state of thread threadId
        static uint32_t* getThreadState(size_t threadId);

        static void     setEpoch(size_t threadId, size_t otherThreadId, uint32_t epoch);

        // increments the epoch of the thread with id threadId
        static void     incrementEpoch(size_t threadId);
        // gets the epoch of the thread with id otherThreadId from the thread with id threadId epoch[threadId][otherThreadId]
        static uint32_t getEpoch(size_t threadId, size_t otherThreadId);
        // sets the epoch of the thread with id otherThreadId from the thread with id threadId to epoch
        static void     maxEpoch(size_t threadId, size_t otherThreadId, uint32_t epoch);  

        static void     transferEpoch(size_t from_tid, size_t to_tid);
        static void     clearEpoch(size_t threadId);
};

#endif