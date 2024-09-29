#ifndef MATSA_THREADPOOL_HPP
#define MATSA_THREADPOOL_HPP

#include "memory/allocation.hpp"

#include <cstdint>

#include "matsaDefs.hpp"

// a circular queue
class ThreadQueue : public CHeapObj<mtInternal> {
    private:
        uint8_t _queue[MAX_THREADS];
        uint16_t _front;
        uint16_t _rear;
        uint8_t _lock;
    public:
        ThreadQueue(void);

        uint8_t enqueue(uint8_t tid);
        int     dequeue(void);
        int     front(void);
        bool    empty(void);

        uint8_t enqueue_unsafe(uint8_t tid);
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