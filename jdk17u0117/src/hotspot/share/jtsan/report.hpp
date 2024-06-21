#ifndef REPORT_HPP
#define REPORT_HPP

#include "stacktrace.hpp"
#include "shadow.hpp"

#include "runtime/mutex.hpp"
#include "oops/method.hpp"
#include "utilities/globalDefinitions.hpp"

namespace JTSanReport {
    extern Mutex _report_lock;

    void do_report_race   (JTSanStackTrace *trace, void *addr, uint8_t size, address bcp, Method *m, 
                            ShadowCell &cur, ShadowCell &prev);
    void print_stack_trace(JTSanStackTrace *trace);
}

#endif