#ifndef MATSA_THREADPOOL_HPP
#define MATSA_THREADPOOL_HPP

#include "memory/allocation.hpp"

#include <cstdint>

#include "matsaDefs.hpp"

// a circular queue
class ThreadQueue : public CHeapObj<mtInternal> {
    private:
        uint64_t *_queue;
        uint64_t _front;
        uint64_t _rear;
        uint8_t _lock;
    public:
        ThreadQueue(uint64_t size = MAX_THREADS);
        ~ThreadQueue(void);

        bool     enqueue(uint64_t tid);
        bool     dequeue_if_not_empty(uint64_t &tid);
        uint64_t dequeue(void);
        uint64_t front(void);
        bool     empty(void);

        uint64_t enqueue_unsafe(uint64_t tid);
};

/*
    * Quarantine is a small deque.
    * It is used to store threads that just exited.
    * Once it gets full, we will start using the threads from the back.
    * It is used to avoid creating new thread metadata for each new thread by reusing the old ones.
*/
class Quarantine : public CHeapObj<mtInternal> {
    private:
        uint64_t *_queue;
        uint64_t _front;
        uint64_t _rear;
        uint64_t _size;
        uint64_t _idx;
        uint8_t _lock;
    public:
        // pretty small so threads can be reused quickly
        Quarantine(uint64_t size = MAX_QUARANTINE_THREADS);
        ~Quarantine(void);

        bool push_back(uint64_t tid);
        bool pop_front(uint64_t &tid);
};


class MaTSaThreadPool : public CHeapObj<mtInternal> {
    private:
        ThreadQueue *_queue;
        Quarantine  *_quarantine;

        MaTSaThreadPool(void);
        ~MaTSaThreadPool(void);

        static MaTSaThreadPool *instance;
    public:
        static MaTSaThreadPool *get_instance(void);

        static void matsa_threadpool_destroy(void);
        static void matsa_threadpool_init(void);

        static ThreadQueue *get_queue(void);
        static Quarantine *get_quarantine(void);
};

#endif