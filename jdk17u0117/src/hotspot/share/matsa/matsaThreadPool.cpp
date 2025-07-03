#include "matsaThreadPool.hpp"
#include "matsaGlobals.hpp"

#include "memory/allocation.hpp"

ThreadQueue::ThreadQueue(uint64_t size) {
    _queue = NEW_C_HEAP_ARRAY(uint64_t, size, mtInternal);

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

bool ThreadQueue::enqueue(uint64_t tid) {
    // we are using a spinlock since Thread::current can return null on vm exit
    // spinlock won't be a problem since we are not going to have a lot of contention anyway
    MaTSaSpinLock lock(&_lock);

    if ((_rear + 1) % MAX_THREADS == _front) {
        return false;
    }

    _queue[_rear] = tid;
    _rear = (_rear + 1) % MAX_THREADS;

    return true;
}

uint64_t ThreadQueue::dequeue(void) {
    MaTSaSpinLock lock(&_lock);

    if (_front == _rear) {
        fatal("ThreadQueue: dequeue from empty queue");
    }

    uint64_t tid = _queue[_front];
    _front = (_front + 1) % MAX_THREADS;

    return tid;
}

bool ThreadQueue::dequeue_if_not_empty(uint64_t &tid) {
    MaTSaSpinLock lock(&_lock);

    if (_front == _rear) {
        return false; // queue is empty
    }

    tid     = _queue[_front];
    _front = (_front + 1) % MAX_THREADS;

    return true;
}

uint64_t ThreadQueue::front(void) {
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

Quarantine::Quarantine(uint64_t size) {
    _queue = NEW_C_HEAP_ARRAY(uint64_t, size, mtInternal);

    assert(_queue != nullptr, "MATSA: Failed to allocate quarantine memory");

    _front = 0;
    _rear  = 0;
    _size  = size;
    _idx   = 0;
    _lock  = 0;
}

Quarantine::~Quarantine(void) {
    assert(_queue != nullptr, "MATSA: Quarantine memory not allocated");

    FREE_C_HEAP_ARRAY(uint64_t, _queue);
    _queue = nullptr;

    _front = 0;
    _rear  = 0;
    _size  = 0;
}

bool Quarantine::push_back(uint64_t tid) {
    MaTSaSpinLock lock(&_lock);

    if ((_rear + 1) % _size == _front) {
        return false; // quarantine is full
    }

    _queue[_rear] = tid;
    _rear = (_rear + 1) % _size;
    _idx++;

    return true;
}

bool Quarantine::pop_front(uint64_t &tid) {
    MaTSaSpinLock lock(&_lock);

    if ((_rear + 1) % _size != _front) {
        return false; // Return false because the queue is not full.
    }

    tid    = _queue[_front];
    _front = (_front + 1) % _size;
    _idx--;

    return true;
}


MaTSaThreadPool* MaTSaThreadPool::instance = nullptr;

MaTSaThreadPool::MaTSaThreadPool(void) {
    _queue      = new ThreadQueue();
    _quarantine = new Quarantine();

    // init the thread stack with all the available tids 0 - MAX_THREADS
    for (int i = 0; i < MAX_THREADS; i++) {
       _queue->enqueue_unsafe(i); 
    }
}

MaTSaThreadPool::~MaTSaThreadPool(void) {
    delete _queue;
    delete _quarantine;
}

Quarantine* MaTSaThreadPool::get_quarantine(void) {
    return instance->_quarantine;
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


