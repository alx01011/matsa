#ifndef HISTORY_HPP
#define HISTORY_HPP

#define MAX_EPOCH_BITS  (16)
#define MAX_BUFFER_BITS (16)
#define MAX_BUFFERS (1 << MAX_BUFFER_BITS)
#define MAX_EVENTS  (1 << MAX_BUFFER_BITS)

#include "runtime/thread.hpp"
#include "oops/method.hpp"
#include "memory/allocation.hpp"

/*
    * History is used to keep track of previous stack traces and is per thread
    * It consists of:
    * - A ring buffer of event buffers
    * Each event buffer keeps function enter and exit events along with their BCI
*/

struct EventBuffer {
    struct Event {
        Method *method;
        uint16_t bci;
    } *events;

    uint64_t *real_stack;
    uint64_t real_stack_size;
    uint64_t epoch : MAX_EPOCH_BITS;
};

class History : public CHeapObj<mtInternal> {
    public:
        static History *history;

        static History *init_history(void);
        // todo destroy

        History();
        ~History();

        static void add_event(JavaThread *thread, Method *m, uint16_t bci);
        static EventBuffer *get_buffer(uint64_t tid, uint64_t idx);
    private:
        EventBuffer *buffer;
        uint64_t buffer_idx : MAX_BUFFER_BITS;
        uint64_t event_idx : MAX_EPOCH_BITS;
};

#endif