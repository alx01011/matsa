#include "report.hpp"
#include "matsaGlobals.hpp"
#include "symbolizer.hpp"
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


MaTSaReportMap *MaTSaReportMap::_instance = nullptr;

MaTSaReportMap::MaTSaReportMap() {
    _size = 0;
    memset(_table, 0, sizeof(_table));
}

MaTSaReportMap::~MaTSaReportMap() {
    clear();
}

// we will use simple fnv1a hash
uint64_t MaTSaReportMap::hash(uintptr_t bcp) {
    assert(sizeof(bcp) == 8, "MaTSa Hash: bcp must be 64 bits");
    assert(bcp, "MaTSa Hash: bcp must be non-zero");

    uint64_t hash  = 0xcbf29ce484222325ull; // offset basis
    uint64_t prime = 0x00000100000001B3ull; // prime 
    uint8_t *ptr = (uint8_t*)&bcp;

    for (size_t i = 0; i < sizeof(bcp); i++) {
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

bool MaTSaReportMap::contains(uintptr_t bcp) {
    uint64_t hash_val = hash(bcp);
    uint64_t bucket   = hash_val % BUCKETS;

    ReportEntry *entry = _table[bucket];
    while (entry != nullptr) {
        if (entry->bcp == bcp) {
            return true;
        }

        entry = entry->next;
    }

    return false;
}

void MaTSaReportMap::insert(uintptr_t bcp) {
    uint64_t hash_val = hash(bcp);
    uint64_t bucket   = hash_val % BUCKETS;

    ReportEntry *entry = _table[bucket];

    ReportEntry *new_entry = NEW_C_HEAP_OBJ(ReportEntry, mtInternal);
    new_entry->bcp  = bcp;
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
    const char *file_name = "<null>";
    InstanceKlass *holder = m->method_holder();
    Symbol *source_file   = nullptr;

    if (holder != nullptr && (source_file = holder->source_file_name()) != nullptr) {
        file_name = source_file->as_C_string();
    }

    const char *method_name = m->external_name_as_fully_qualified();
    const int  lineno       = m->line_number_from_bci(bci);


    fprintf(stderr, "  #%d %s() %s:%d\n", index, method_name, file_name, lineno);
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

    for (int i = stack_size - 1; i >= 0; i--, prev_frame = raw_frame) {
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

}

struct EventTrace {
    Method *m;
    uint16_t bci;
};

// potentially we can do a bit better here in the future (code is not that clean)
bool try_print_event_trace(void *addr, int tid, ShadowCell &cell, HistoryCell &prev_history) {
    // MaTSaEventTrace trace;
    EventTrace *trace = NEW_C_HEAP_ARRAY(EventTrace, MAX_EVENTS + DEFAULT_STACK_SIZE, mtInternal);
    uint64_t    trace_idx = 0;
    bool has_trace = false;

    History *h = History::get_history(tid);
    EventBuffer *buffer = h->get_buffer(tid, prev_history.ring_idx);

    for (uint64_t i = 0; i < prev_history.history_idx; i++) {
        if (buffer->events[i].method == 0) {
            trace_idx--;
            continue;
        }

        trace[trace_idx].m   = buffer->events[i].method;
        trace[trace_idx].bci = buffer->events[i].bci;
        trace_idx++;
    }

    int prev_bci = prev_history.bci;
    for (int64_t i = trace_idx - 1; i >= 0; i--) {
        print_method_info(trace[i].m, prev_bci, i);

        prev_bci = trace[i].bci;
    }



    // has_trace = Symbolizer::TraceUpToAddress(trace, addr, tid, cell);

    // if (has_trace) {
    //     for (int i = trace.size - 1; i >= 0; i--) {
    //         MaTSaEvent e = trace.events[i];
    //         Method *m    = (Method*)((uintptr_t)e.pc);
            
    //         if (!Method::is_valid_method(m)) {
    //             continue;
    //         }

    //         if (i != trace.size - 1) {
    //             e.bci = trace.events[i + 1].bci;
    //         }

    //         print_method_info(m, e.bci, (trace.size - 1) - i);
    //     }
    // }

    return has_trace;
}

void MaTSaReport::do_report_race(JavaThread *thread, void *addr, uint8_t size, address bcp, Method *m, 
                            ShadowCell &cur, ShadowCell &prev, HistoryCell &prev_history) {
    MutexLocker ml(MaTSaReport::_report_lock, Mutex::_no_safepoint_check_flag);

    // already reported
    if (MaTSaReportMap::instance()->contains((uintptr_t)bcp)) {
        return;
    }

    // is suppressed?
    if (LIKELY(MaTSaSuppression::is_suppressed(thread))) {
        MaTSaReportMap::instance()->insert((uintptr_t)bcp);
        return;
    }

    
    int pid = os::current_process_id();
    int cur_bci = m->bci_from(bcp);
    ResourceMark rm;

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

    const char *file_name   = "<null>";
    const char *method_name = m->external_name_as_fully_qualified();
    const int lineno        = m->line_number_from_bci(cur_bci);

    InstanceKlass *holder = m->method_holder();
    Symbol *source_file   = nullptr;
    if (holder != nullptr && (source_file = holder->source_file_name()) != nullptr) {
        file_name = source_file->as_C_string();
    }

    fprintf(stderr, "\nSUMMARY: ThreadSanitizer: data race %s:%d in %s()\n", file_name, lineno, method_name);
    fprintf(stderr, "==================\n");

    // store it in the report map
    MaTSaReportMap::instance()->insert((uintptr_t)bcp);

    COUNTER_INC(race);
}