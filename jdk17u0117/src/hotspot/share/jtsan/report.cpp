#include "report.hpp"

#include "runtime/os.hpp"

#include <cstdint>

#define RED "\033[1;31m"
#define BLUE "\033[1;34m"
#define RESET "\033[0m"

Mutex JTSanReport::_report_lock(Mutex::leaf, "JTSanReport::_report_lock");

// must hold lock else the output will be garbled
void JTSanReport::print_stack_trace(JTSanStackTrace *trace) {
    for (size_t i = 0; i < trace->frame_count(); i++) {
        JTSanStackFrame frame = trace->get_frame(i);
        Method *method = frame.method;
        address pc = frame.pc;

        const char    *file_name   = nullptr;
        InstanceKlass *holder      = method->method_holder();
        Symbol        *source_file = nullptr;

        if (holder != nullptr && (source_file = holder->source_file_name()) != nullptr){
            file_name = source_file->as_C_string();
        }

        const char *method_name = method->external_name_as_fully_qualified();
        const int lineno        = method->line_number_from_bci(method->bci_from(pc));

        fprintf(stderr, "  #%zu %s %s:%d\n", i, method_name, file_name, lineno);
    }

}

void JTSanReport::do_report_race(JTSanStackTrace *trace, void *addr, uint8_t size, address bcp, Method *m, 
                            ShadowCell &cur, ShadowCell &prev) {
    JTSanReport::_report_lock.lock();
    
    int pid = os::current_process_id();

    fprintf(stderr, "==================\n");

    fprintf(stderr, RED "WARNING: ThreadSanitizer: data race (pid=%d)\n", pid);
    fprintf(stderr, BLUE " %s of size %u at %p by thread T%lu:\n" RESET,  cur.is_write ? "Write" : "Read", size, addr, cur.tid);
    print_stack_trace(trace);
    
    fprintf(stderr, BLUE "\n Previous %s of size %u at %p by thread T%lu:\n" RESET, prev.is_write ? "write" : "read", size, addr, prev.tid);
    // TODO: find previous stack trace

    const char *file_name   = m->method_holder()->source_file_name()->as_C_string();
    const char *method_name = m->external_name_as_fully_qualified();
    const int lineno        = m->line_number_from_bci(m->bci_from(bcp));

    fprintf(stderr, "\nSUMMARY: ThreadSanitizer: data race %s:%d in %s\n", "file", 0, "method");
    fprintf(stderr, "==================\n");
}