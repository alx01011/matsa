#include "matsaThreadPool.hpp"
#include "matsaGlobals.hpp"

#include "memory/allocation.hpp"

ThreadQueue::ThreadQueue(void) {
    _queue = NEW_C_HEAP_ARRAY(uint64_t, MAX_THREADS, mtInternal);

    assert(_queue != nullptr, "MATSA: Failed to allocate thread queue memory");

    _front = 0;
    _rear  = 0;
    _lock  = 0;
}

ThreadQueue::~ThreadQueue(void) {
    assert(_queue != nullptr, "MATSA: Thread queue memory not allocated");

    FREE_C_HEAP_ARRAY(uint64_t, _queue);
    _queue = nullptr;

    _front = 0;
    _rear  = 0;
}

uint64_t ThreadQueue::enqueue(uint64_t tid) {
    // we are using a spinlock since Thread::current can return null on vm exit
    // spinlock won't be a problem since we are not going to have a lot of contention anyway
    MaTSaSpinLock lock(&_lock);

    if ((_rear + 1) % MAX_THREADS == _front) {
        return 1;
    }

    _queue[_rear] = tid;
    _rear = (_rear + 1) % MAX_THREADS;

    return 0;
}

int ThreadQueue::dequeue(void) {
    MaTSaSpinLock lock(&_lock);

    if (_front == _rear) {
        return -1;
    }

    uint64_t tid = _queue[_front];
    _front = (_front + 1) % MAX_THREADS;

    return tid;
}

int ThreadQueue::front(void) {
    MaTSaSpinLock lock(&_lock);

    return _queue[_front];
}

bool ThreadQueue::empty(void) {
    MaTSaSpinLock lock(&_lock);

    return _front == _rear;
}

uint64_t ThreadQueue::enqueue_unsafe(uint64_t tid) {
    if ((_rear + 1) % MAX_THREADS == _front) {
        return 1;
    }

    _queue[_rear] = tid;
    _rear = (_rear + 1) % MAX_THREADS;

    return 0;
}


MaTSaThreadPool* MaTSaThreadPool::instance = nullptr;

MaTSaThreadPool::MaTSaThreadPool(void) {
    _queue = new ThreadQueue();

    // init the thread stack with all the available tids 0 - MAX_THREADS
    for (int i = 0; i < MAX_THREADS; i++) {
       _queue->enqueue_unsafe(i); 
    }
}

MaTSaThreadPool::~MaTSaThreadPool(void) {
    delete _queue;
}

ThreadQueue* MaTSaThreadPool::get_queue(void) {
    return instance->_queue;
}

MaTSaThreadPool* MaTSaThreadPool::get_instance(void) {
    if (UNLIKELY(instance == nullptr)) {
        instance = new MaTSaThreadPool();
    }

    return instance;
}

void MaTSaThreadPool::matsa_threadpool_init(void) {
    if (UNLIKELY(instance != nullptr)) {
        return;
    }
    instance = new MaTSaThreadPool();
}

void MaTSaThreadPool::matsa_threadpool_destroy(void) {
    if (LIKELY(instance != nullptr)) {
        delete instance;
    }
}


