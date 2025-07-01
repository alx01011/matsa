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
        ThreadQueue(void);
        ~ThreadQueue(void);

        uint64_t enqueue(uint64_t tid);
        int     dequeue(void);
        int     front(void);
        bool    empty(void);

        uint64_t enqueue_unsafe(uint64_t tid);
};


class MaTSaThreadPool : public CHeapObj<mtInternal> {
    private:
        ThreadQueue* _queue;

        MaTSaThreadPool(void);
        ~MaTSaThreadPool(void);

        static MaTSaThreadPool* instance;
    public:
        static MaTSaThreadPool* get_instance(void);

        static void matsa_threadpool_destroy(void);
        static void matsa_threadpool_init(void);

        static ThreadQueue* get_queue(void);
};

#endif