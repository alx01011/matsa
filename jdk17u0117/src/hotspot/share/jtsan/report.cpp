#include "report.hpp"
#include "jtsanGlobals.hpp"
#include "symbolizer.hpp"

#include "runtime/os.hpp"

#include <cstdint>

#define RED   "\033[1;31m"
#define BLUE  "\033[1;34m"
#define RESET "\033[0m"

Mutex *JTSanReport::_report_lock;

void print_method_info(Method *m, int bci, int index) {
    const char *file_name = "<null>";
    InstanceKlass *holder = m->method_holder();
    Symbol *source_file   = nullptr;

    if (holder != nullptr && (source_file = holder->source_file_name()) != nullptr) {
        file_name = source_file->as_C_string();
    }

    const char *method_name = m->external_name_as_fully_qualified();
    const int lineno        = m->line_number_from_bci(bci);

    fprintf(stderr, "  #%d %s() %s:%d\n", index, method_name, file_name, lineno);
}


// must hold lock else the output will be garbled
void JTSanReport::print_stack_trace(JTSanStackTrace *trace) {
    for (size_t i = 0; i < trace->frame_count(); i++) {
        JTSanStackFrame frame = trace->get_frame(i);
        Method *method = frame.method;
        address pc = frame.pc;

        int bci = method->bci_from(pc);
        print_method_info(method, bci, i);
    }

}

bool try_print_event_trace(void *addr, int tid, ShadowCell &prev) {
    JTSanEventTrace trace;
    bool has_trace = false;

    has_trace = Symbolizer::TraceUpToAddress(trace, addr, tid, prev);

    if (has_trace) {
        for (int i = trace.size - 1; i >= 0; i--) {
            JTSanEvent e = trace.events[i];
            
            uintptr_t pc    =  e.pc;

            jmethodID mid   = (jmethodID)((uintptr_t)pc);
            Method *m       = Method::resolve_jmethod_id(mid);

            int bci         = e.bci;

            print_method_info(m, bci, (trace.size - 1) - i);
        }
    }

    return has_trace;
}

void JTSanReport::do_report_race(JTSanStackTrace *trace, void *addr, uint8_t size, address bcp, Method *m, 
                            ShadowCell &cur, ShadowCell &prev) {
    JTSanScopedLock lock(JTSanReport::_report_lock);

    
    int pid = os::current_process_id();

    fprintf(stderr, "==================\n");

    fprintf(stderr, RED "WARNING: ThreadSanitizer: data race (pid=%d)\n", pid);
    fprintf(stderr, BLUE " %s of size %u at %p by thread T%u:\n" RESET,  cur.is_write ? "Write" : "Read", 
            size, addr, (uint32_t)cur.tid);
    if (!try_print_event_trace(addr, cur.tid, cur)) {
        // less accurate line numbers
        print_stack_trace(trace);
    }
    
    fprintf(stderr, BLUE "\n Previous %s of size %u at %p by thread T%u:\n" RESET, prev.is_write ? "write" : "read", 
            size, addr, (uint32_t)prev.tid);
    if (!try_print_event_trace(addr, prev.tid, prev)) {
        fprintf(stderr, "  <no stack trace available>\n");
    }

    // null checks here are not necessary, at least thats what tests have shown so far
    const char *file_name   = m->method_holder()->source_file_name()->as_C_string();
    const char *method_name = m->external_name_as_fully_qualified();
    const int lineno        = m->line_number_from_bci(m->bci_from(bcp));

    fprintf(stderr, "\nSUMMARY: ThreadSanitizer: data race %s:%d in %s()\n", file_name, lineno, method_name);
    fprintf(stderr, "==================\n");

    COUNTER_INC(race);
}