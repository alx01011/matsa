#include "jtsanThreadPool.hpp"

ThreadQueue::ThreadQueue(void) {
    _front = 0;
    _rear = 0;
    _lock = new Mutex(Mutex::event, "JTSAN Thread Queue Lock");
}

ThreadQueue::~ThreadQueue(void) {
    delete _lock;
}

uint8_t ThreadQueue::enqueue(uint8_t tid) {
    _lock->lock();

    if ((_rear + 1) % MAX_THREADS == _front) {
        _lock->unlock();
        return 1;
    }

    _queue[_rear] = tid;
    _rear = (_rear + 1) % MAX_THREADS;
    _lock->unlock();

    return 0;
}

int16_t ThreadQueue::dequeue(void) {
    _lock->lock();

    if (_front == _rear) {
        _lock->unlock();
        return -1;
    }

    uint8_t tid = _queue[_front];
    _front = (_front + 1) % MAX_THREADS;
    _lock->unlock();

    return tid;
}

int16_t ThreadQueue::front(void) {
    _lock->lock();
    uint8_t tid = _queue[_front];
    _lock->unlock();
    return tid;
}

bool ThreadQueue::empty(void) {
    _lock->lock();
    bool empty = _front == _rear;
    _lock->unlock();
    return empty;
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
    // we are skipping 0 as it is reserved for the main thread
    for (int i = 1; i < MAX_THREADS; i++) {
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


