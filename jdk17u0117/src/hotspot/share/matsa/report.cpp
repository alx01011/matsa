#include "report.hpp"
#include "matsaGlobals.hpp"
#include "symbolizer.hpp"
#include "matsaStack.hpp"

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


//uint8_t MaTSaReport::_report_lock;
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

    for (int i = stack_size - 1; i >= 0; i--) {
        uint64_t raw_frame = stack->get(i);
        uint64_t raw_sender = i > 0 ? stack->get(i - 1) : 0;
        mp  = raw_sender ? (Method*)(raw_sender >> 16) : (Method*)(raw_frame >> 16);
        bci = (i == stack_size - 1 ? cur_bci : raw_frame & 0xFFFF);

        print_method_info(mp, bci, (stack_size - 1) - i);
    }

}

bool try_print_event_trace(void *addr, int tid, ShadowCell &cell) {
    MaTSaEventTrace trace;
    bool has_trace = false;

    has_trace = Symbolizer::TraceUpToAddress(trace, addr, tid, cell);

    if (has_trace) {
        for (int i = trace.size - 1; i >= 0; i--) {
            MaTSaEvent e = trace.events[i];
            Method *m    = (Method*)((uintptr_t)e.pc);
            
            if (!Method::is_valid_method(m)) {
                continue;
            }

            print_method_info(m, e.bci, (trace.size - 1) - i);
        }
    }

    return has_trace;
}

void MaTSaReport::do_report_race(JavaThread *thread, void *addr, uint8_t size, address bcp, Method *m, 
                            ShadowCell &cur, ShadowCell &prev) {
    MutexLocker ml(MaTSaReport::_report_lock, Mutex::_no_safepoint_check_flag);

    // already reported
    if (MaTSaReportMap::instance()->contains((uintptr_t)bcp)) {
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
    if (!try_print_event_trace(addr, prev.tid, prev)) {
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