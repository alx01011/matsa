#ifndef HISTORY_HPP
#define HISTORY_HPP

#define MAX_EPOCH_BITS  (16)
#define MAX_EVENT_BITS  (16)
#define MAX_PART_BITS   (16)
#define MAX_PARTS   (1 << MaTSaHistorySize)
#define MAX_EVENTS  (1 << MAX_EVENT_BITS)

#include "matsaDefs.hpp"

#include "runtime/thread.hpp"
#include "oops/method.hpp"
#include "memory/allocation.hpp"

/*
    * History is used to keep track of previous stack traces and is per thread
    * It consists of:
    * - A ring buffer of event buffers
    * Each event buffer keeps function enter and exit events along with their BCI
*/

struct Event {
    uint64_t method : MAX_ADDRESS_BITS;
    uint64_t bci    : MAX_BCI_BITS;
};

struct Part {
    Event *events;
    uint64_t event_idx       : MAX_EVENT_BITS;
    uint64_t epoch           : MAX_EPOCH_BITS;
    uint64_t real_stack      : MAX_ADDRESS_BITS;
    uint64_t real_stack_size : MAX_STACK_BITS;
};

class History : public CHeapObj<mtInternal> {
    public:
        // todo instead of having an array of history objects
        // we can store the history object inside each thread
        // when looking for a previous stack trace we would somehow need to get the thread pointer
        // it could be placd inside the threadpool
        static History **history;

        static void init_history(void);
        // todo destroy

        History();
        ~History();

        static void add_event(JavaThread *thread, Method *m, uint16_t bci);
        static Part *get_part(uint64_t tid, uint64_t idx);

        static History *get_history(uint64_t tid);
        static void     reset(uint64_t tid);

        static uint64_t get_part_idx(uint64_t tid);
        static uint64_t get_event_idx(uint64_t tid);
        static uint64_t get_cur_epoch(uint64_t tid);
    private:
        Part *parts;
        uint64_t part_idx  : MAX_PART_BITS;
        uint64_t event_idx : MAX_EVENT_BITS;
};

#endif