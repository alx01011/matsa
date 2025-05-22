#include "report.hpp"
#include "matsaGlobals.hpp"
#include "suppression.hpp"
#include "matsaStack.hpp"
#include "history.hpp"

#include "runtime/os.hpp"
#include "runtime/mutexLocker.hpp"
#include "oops/method.hpp"
#include "memory/resourceArea.hpp"
#include "utilities/debug.hpp"

#include <cstdint>

#define RED   "\033[1;31m"
#define BLUE  "\033[1;34m"
#define RESET "\033[0m"

// print up to 64 frames
#define MAX_FRAMES (64)


MaTSaReportMap *MaTSaReportMap::_instance = nullptr;

MaTSaReportMap::MaTSaReportMap() {
    _size = 0;
    memset(_table, 0, sizeof(_table));
}

MaTSaReportMap::~MaTSaReportMap() {
    clear();
}

// we will use simple fnv1a hash
uint64_t MaTSaReportMap::hash(uint64_t method) {
    assert(sizeof(method) == 8, "MaTSa Hash: method pointer must be 64 bits");
    assert(method, "MaTSa Hash: method pointer must be non-zero");

    uint64_t hash  = 0xcbf29ce484222325ull; // offset basis
    uint64_t prime = 0x00000100000001B3ull; // prime 
    uint8_t *ptr = (uint8_t*)&method;

    for (size_t i = 0; i < sizeof(method); i++) {
        hash ^= ptr[i];
        hash *= prime;
    }

    return hash;
}

MaTSaReportMap *MaTSaReportMap::instance() {
    assert(_instance != nullptr, "MaTSaReportMap not initialized");
    return _instance;
}

void MaTSaReportMap::init() {
    assert(_instance == nullptr, "MaTSaReportMap already initialized");
    _instance = new MaTSaReportMap();
}

void MaTSaReportMap::destroy() {
    assert(_instance != nullptr, "MaTSaReportMap not initialized");
    _instance->clear();

    delete _instance;
    _instance = nullptr;
}

bool MaTSaReportMap::contains(uint64_t method, uint64_t bci) {
    uint64_t hash_val = hash(method);
    uint64_t bucket   = hash_val % BUCKETS;

    ReportEntry *entry = _table[bucket];
    while (entry != nullptr) {
        if (entry->bci == bci) {
            return true;
        }

        entry = entry->next;
    }

    return false;
}

void MaTSaReportMap::insert(uint64_t method, uint64_t bci) {
    uint64_t hash_val = hash(method);
    uint64_t bucket   = hash_val % BUCKETS;

    ReportEntry *entry = _table[bucket];

    ReportEntry *new_entry = NEW_C_HEAP_OBJ(ReportEntry, mtInternal);
    new_entry->bci  = bci;
    new_entry->next = entry;

    _table[bucket] = new_entry;
    _size++;
}

void MaTSaReportMap::clear() {
    for (size_t i = 0; i < BUCKETS; i++) {
        ReportEntry *entry = _table[i];
        while (entry != nullptr) {
            ReportEntry *next = entry->next;
            FREE_C_HEAP_OBJ(entry);
            entry = next;
        }
    }

    _size = 0;
}

Mutex *MaTSaReport::_report_lock;

void print_method_info(Method *m, int bci, int index) {
    char methodname_buf[256] = {"??"};
    char filename_buf[128] = {"??"};
    InstanceKlass *holder = m->method_holder();
    Symbol *source_file   = nullptr;

    if (holder != nullptr && (source_file = holder->source_file_name()) != nullptr) {
        source_file->as_C_string(filename_buf, sizeof(filename_buf));
    }

    m->name_and_sig_as_C_string(methodname_buf, sizeof(methodname_buf));

    int lineno = m->line_number_from_bci(bci);

    if (lineno == -1) {
        fprintf(stderr, "  #%d %s %s:??\n", index, methodname_buf, filename_buf);
        return;
    }

    fprintf(stderr, "  #%d %s %s:%d\n", index, methodname_buf, filename_buf, lineno);
}

// must hold lock else the output will be garbled
void MaTSaReport::print_current_stack(JavaThread *thread, int cur_bci) {
    MaTSaStack *stack = JavaThread::get_matsa_stack(thread);

    int stack_size = stack->size();
    Method *mp = NULL;
    int bci = 0;

    // encode the current bci
    uint64_t prev_frame = 0 | cur_bci;
    uint64_t raw_frame  = 0;

    for (int i = stack_size - 1, count = 0; i >= 0 && count < MAX_FRAMES; i--, prev_frame = raw_frame, count++) {
        raw_frame = stack->get(i);

        mp  = (Method*)(raw_frame >> 16);

        /*  
            bci handling is a bit tricky:
            - if we are at the top of the stack, then the bci is the current bci
                - that is because the current bci is the bci of the access that caused the race
                - obviously the access was made in the current method
            - if we are not at the top of the stack, then the bci is the bci of the sender
                - that is because the sender is the method that called the current method
                - we want to know the line number the sender called the current method
        */
        bci = prev_frame & 0xFFFF;

        print_method_info(mp, bci, (stack_size - 1) - i);
    }

    if (stack_size > MAX_FRAMES) {
        fprintf(stderr, "  <truncated>\n");
    }

}

struct EventTrace {
    Method *m;
    uint64_t bci : 16;
};

// potentially we can do a bit better here in the future (code is not that clean)
bool try_print_event_trace(void *addr, int tid, ShadowCell &cell, HistoryCell &prev_history) {
    // MaTSaEventTrace trace;
    EventTrace *trace = NEW_C_HEAP_ARRAY(EventTrace, MAX_EVENTS + DEFAULT_STACK_SIZE, mtInternal);
    uint64_t    trace_idx = 0;
    bool has_trace = false;

    History *h = History::get_history(tid);
    Part *part = h->get_part(tid, prev_history.ring_idx);

    if (prev_history.history_epoch != part->epoch) {
        FREE_C_HEAP_ARRAY(EventTrace, trace);
        return false;
    }

    uint64_t *real_stack = (uint64_t*)((uint64_t)part->real_stack);

    for (uint64_t i = 0; i < part->real_stack_size; i++) {
        Method *m = (Method*)(real_stack[i] >> 16);

        trace[trace_idx].m   = m;
        trace[trace_idx].bci = real_stack[i] & 0xFFFF;
        trace_idx++;
    }

    for (uint64_t i = 0; i < prev_history.history_idx; i++) {
        if (part->events[i].method == 0) {
            trace_idx -= trace_idx > 0;
            continue;
        }

        trace[trace_idx].m   = (Method*)((uint64_t)part->events[i].method);
        trace[trace_idx].bci = part->events[i].bci;
        trace_idx++;
    }

    int prev_bci = prev_history.bci;
    for (int64_t i = trace_idx - 1, count = 0; i >= 0 && count < MAX_FRAMES; i--, count++) {
        print_method_info(trace[i].m, prev_bci, (trace_idx - 1) - i);
        prev_bci = trace[i].bci;
    }

    if (trace_idx > MAX_FRAMES) {
        fprintf(stderr, "  <truncated>\n");
    }

    FREE_C_HEAP_ARRAY(EventTrace, trace);
    return trace_idx != 0;
}

void MaTSaReport::do_report_race(JavaThread *thread, void *addr, uint8_t size, int cur_bci, Method *m, ShadowCell &cur, 
                                ShadowCell &prev, HistoryCell &prev_history) {
    MutexLocker ml(MaTSaReport::_report_lock, Mutex::_no_safepoint_check_flag);

    // already reported
    if (MaTSaReportMap::instance()->contains((uint64_t)m, (uint64_t)cur_bci)) {
        return;
    }

    // is suppressed?
    if (LIKELY(MaTSaSuppression::is_suppressed(thread))) {
        MaTSaReportMap::instance()->insert((uint64_t)m, (uint64_t)cur_bci);
        return;
    }

    
    int pid = os::current_process_id();
    // ResourceMark rm;

    fprintf(stderr, "==================\n");

    fprintf(stderr, RED "WARNING: ThreadSanitizer: data race (pid=%d)\n", pid);
    fprintf(stderr, BLUE " %s of size %u at %p by thread T%u:" RESET"\n",  cur.is_write ? "Write" : "Read", 
            size, addr, (uint32_t)cur.tid);

    print_current_stack(thread, cur_bci);
    
    fprintf(stderr, BLUE "\n Previous %s of size %u at %p by thread T%u:" RESET"\n", prev.is_write ? "write" : "read", 
            size, addr, (uint32_t)prev.tid);
    if (!try_print_event_trace(addr, prev.tid, prev, prev_history)) {
        fprintf(stderr, "  <no stack trace available>\n");
    }

    

    char methodname_buf[256] = {"??"};
    char filename_buf[128] = {"??"};

    const int lineno        = m->line_number_from_bci(cur_bci);

    InstanceKlass *holder = m->method_holder();
    Symbol *source_file   = nullptr;
    if (holder != nullptr && (source_file = holder->source_file_name()) != nullptr) {
        source_file->as_C_string(filename_buf, sizeof(filename_buf));
    }

    m->name_and_sig_as_C_string(methodname_buf, sizeof(methodname_buf));

    fprintf(stderr, "\nSUMMARY: ThreadSanitizer: data race %s:%d in %s\n", filename_buf, lineno, methodname_buf);
    fprintf(stderr, "==================\n");

    // store it in the report map
    MaTSaReportMap::instance()->insert((uint64_t)m, (uint64_t)cur_bci);

    COUNTER_INC(race);
}