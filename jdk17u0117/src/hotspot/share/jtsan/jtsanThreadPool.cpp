#include "jtsanThreadPool.hpp"
#include "jtsanGlobals.hpp"

ThreadQueue::ThreadQueue(void) {
    _front = 0;
    _rear  = 0;
    _lock  = 0;
}

uint8_t ThreadQueue::enqueue(uint8_t tid) {
    // we are using a spinlock since Thread::current can return null on vm exit
    // spinlock won't be a problem since we are not going to have a lot of contention anyway
    JTSanSpinLock lock(&_lock);

    if ((_rear + 1) % MAX_THREADS == _front) {
        return 1;
    }

    _queue[_rear] = tid;
    _rear = (_rear + 1) % MAX_THREADS;

    return 0;
}

int ThreadQueue::dequeue(void) {
    JTSanSpinLock lock(&_lock);

    if (_front == _rear) {
        return -1;
    }

    uint8_t tid = _queue[_front];
    _front = (_front + 1) % MAX_THREADS;

    return tid;
}

int ThreadQueue::front(void) {
    JTSanSpinLock lock(&_lock);

    return _queue[_front];
}

bool ThreadQueue::empty(void) {
    JTSanSpinLock lock(&_lock);

    return _front == _rear;
}

uint8_t ThreadQueue::enqueue_unsafe(uint8_t tid) {
    if ((_rear + 1) % MAX_THREADS == _front) {
        return 1;
    }

    _queue[_rear] = tid;
    _rear = (_rear + 1) % MAX_THREADS;

    return 0;
}


JtsanThreadPool* JtsanThreadPool::instance = nullptr;

JtsanThreadPool::JtsanThreadPool(void) {
    _queue = new ThreadQueue();

    // init the thread stack with all the available tids 0 - 255
    for (int i = 0; i < MAX_THREADS; i++) {
       _queue->enqueue_unsafe(i); 
    }
}

JtsanThreadPool::~JtsanThreadPool(void) {
    delete _queue;
}

ThreadQueue* JtsanThreadPool::get_queue(void) {
    return instance->_queue;
}

JtsanThreadPool* JtsanThreadPool::get_instance(void) {
    if (UNLIKELY(instance == nullptr)) {
        instance = new JtsanThreadPool();
    }

    return instance;
}

void JtsanThreadPool::jtsan_threadpool_init(void) {
    if (UNLIKELY(instance != nullptr)) {
        return;
    }
    instance = new JtsanThreadPool();
}

void JtsanThreadPool::jtsan_threadpool_destroy(void) {
    if (LIKELY(instance != nullptr)) {
        delete instance;
    }
}


