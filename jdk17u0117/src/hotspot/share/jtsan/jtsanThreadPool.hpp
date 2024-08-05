#ifndef JTSAN_THREADPOOL_HPP

#include "runtime/mutex.hpp"
#include "memory/allocation.hpp"

#include <cstdint>

#include "jtsanDefs.hpp"

// a circular queue
class ThreadQueue : public CHeapObj<mtInternal> {
    private:
        uint8_t _queue[MAX_THREADS];
        uint16_t _front;
        uint16_t _rear;
        uint8_t _lock;
        // Mutex* _lock;
    public:
        ThreadQueue(void);
        // ~ThreadQueue(void);

        uint8_t enqueue(uint8_t tid);
        int     dequeue(void);
        int     front(void);
        bool    empty(void);

        uint8_t enqueue_unsafe(uint8_t tid);
};


class JtsanThreadPool : public CHeapObj<mtInternal> {
    private:
        ThreadQueue* _queue;

        JtsanThreadPool(void);
        ~JtsanThreadPool(void);

        static JtsanThreadPool* instance;
    public:
        static JtsanThreadPool* get_instance(void);

        static void jtsan_threadpool_destroy(void);
        static void jtsan_threadpool_init(void);

        static ThreadQueue* get_queue(void);
};

#endif