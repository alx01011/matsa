#include "report.hpp"
#include "jtsanGlobals.hpp"
#include "symbolizer.hpp"
#include "jtsanStack.hpp"

#include "runtime/os.hpp"
#include "utilities/debug.hpp"

#include <cstdint>

#define RED   "\033[1;31m"
#define BLUE  "\033[1;34m"
#define RESET "\033[0m"


JTSanReportMap *JTSanReportMap::_instance = nullptr;

JTSanReportMap::JTSanReportMap() {
    _size = 0;
    memset(_table, 0, sizeof(_table));
}

JTSanReportMap::~JTSanReportMap() {
    clear();
}

// we will use simple fnv1a hash
uint64_t JTSanReportMap::hash(uintptr_t bcp) {
    assert(sizeof(bcp) == 8, "JTSan Hash: bcp must be 64 bits");
    assert(bcp, "JTSan Hash: bcp must be non-zero");

    uint64_t hash  = 0xcbf29ce484222325ull; // offset basis
    uint64_t prime = 0x00000100000001B3ull; // prime 
    uint8_t *ptr = (uint8_t*)&bcp;

    for (size_t i = 0; i < sizeof(bcp); i++) {
        hash ^= ptr[i];
        hash *= prime;
    }

    return hash;
}

JTSanReportMap *JTSanReportMap::instance() {
    assert(_instance != nullptr, "JTSanReportMap not initialized");
    return _instance;
}

void JTSanReportMap::init() {
    assert(_instance == nullptr, "JTSanReportMap already initialized");
    _instance = new JTSanReportMap();
}

void JTSanReportMap::destroy() {
    assert(_instance != nullptr, "JTSanReportMap not initialized");
    _instance->clear();

    delete _instance;
    _instance = nullptr;
}

bool JTSanReportMap::contains(uintptr_t bcp) {
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

void JTSanReportMap::insert(uintptr_t bcp) {
    uint64_t hash_val = hash(bcp);
    uint64_t bucket   = hash_val % BUCKETS;

    ReportEntry *entry = _table[bucket];

    ReportEntry *new_entry = NEW_C_HEAP_OBJ(ReportEntry, mtInternal);
    new_entry->bcp  = bcp;
    new_entry->next = entry;

    _table[bucket] = new_entry;
    _size++;
}

void JTSanReportMap::clear() {
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



uint8_t JTSanReport::_report_lock;

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
void JTSanReport::print_stack_trace(JTSanStackTrace *trace) {
    // for (size_t i = 0; i < trace->frame_count(); i++) {
    //     JTSanStackFrame frame = trace->get_frame(i);
    //     Method *method = frame.method;
    //     address pc = frame.pc;

    //     int bci = method->bci_from(pc);
    //     print_method_info(method, bci, i);
    // }

    JavaThread *thread = JavaThread::current();
    JTSanStack *stack = JavaThread::get_jtsan_stack(thread);

    size_t stack_size = stack->size();
    Method *mp = NULL;
    int bci = 0;

    for (ssize_t i = stack_size - 1; i >= 0; i--) {
        uint64_t raw_frame = stack->get(i);
        mp  = (Method*)(raw_frame >> 16);
        bci = raw_frame & 0xFFFF;

        print_method_info(mp, bci, i);
    }

}

bool try_print_event_trace(void *addr, int tid, ShadowCell &cell, void *cell_shadow_addr) {
    JTSanEventTrace trace;
    bool has_trace = false;

    has_trace = Symbolizer::TraceUpToAddress(trace, addr, tid, cell, cell_shadow_addr);

    if (has_trace) {
        for (int i = trace.size - 1; i >= 0; i--) {
            JTSanEvent e = trace.events[i];
            Method *m    = (Method*)((uintptr_t)e.pc);
            
            if (!Method::is_valid_method(m)) {
                continue;
            }

            print_method_info(m, e.bci, (trace.size - 1) - i);
        }
    }

    return has_trace;
}

void JTSanReport::do_report_race(JTSanStackTrace *trace, void *addr, uint8_t size, address bcp, Method *m, 
                            ShadowCell &cur, ShadowCell &prev, ShadowPair &pair) {
    //JTSanScopedLock lock(JTSanReport::_report_lock);
    JTSanSpinLock lock(&_report_lock);

    // already reported
    if (JTSanReportMap::instance()->contains((uintptr_t)bcp)) {
        return;
    }

    
    int pid = os::current_process_id();
    ResourceMark rm;

    fprintf(stderr, "==================\n");

    fprintf(stderr, RED "WARNING: ThreadSanitizer: data race (pid=%d)\n", pid);
    fprintf(stderr, BLUE " %s of size %u at %p by thread T%u:\n" RESET,  cur.is_write ? "Write" : "Read", 
            size, addr, (uint32_t)cur.tid);
    // if (!try_print_event_trace(addr, cur.tid, cur, pair.cur_shadow)) {
    //     // less accurate line numbers
    //     print_stack_trace(trace);
    // }
    if (true) {
        // less accurate line numbers
        print_stack_trace(trace);
    }
    
    fprintf(stderr, BLUE "\n Previous %s of size %u at %p by thread T%u:\n" RESET, prev.is_write ? "write" : "read", 
            size, addr, (uint32_t)prev.tid);
    if (!try_print_event_trace(addr, prev.tid, prev, pair.prev_shadow)) {
        fprintf(stderr, "  <no stack trace available>\n");
    }

    const char *file_name   = "<null>";
    const char *method_name = m->external_name_as_fully_qualified();
    const int lineno        = m->line_number_from_bci(m->bci_from(bcp));

    InstanceKlass *holder = m->method_holder();
    Symbol *source_file   = nullptr;
    if (holder != nullptr && (source_file = holder->source_file_name()) != nullptr) {
        file_name = source_file->as_C_string();
    }

    fprintf(stderr, "\nSUMMARY: ThreadSanitizer: data race %s:%d in %s()\n", file_name, lineno, method_name);
    fprintf(stderr, "==================\n");

    // store it in the report map
    JTSanReportMap::instance()->insert((uintptr_t)bcp);

    COUNTER_INC(race);
}