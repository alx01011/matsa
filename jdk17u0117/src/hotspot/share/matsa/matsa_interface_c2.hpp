#ifndef MATSA_INTERFACE_C2_HPP
#define MATSA_INTERFACE_C2_HPP

#include "oops/method.hpp"
#include "runtime/thread.hpp"
#include "utilities/globalDefinitions.hpp"

namespace MaTSaC2 {
    void method_enter(JavaThread *thread, Method *method, int bci);
    void method_exit(JavaThread *thread, Method *method, int bci);

    void pre_method_enter(JavaThread *current, int bci);

    void sync_enter(JavaThread *thread, oopDesc *obj);
    void sync_exit(JavaThread *thread, oopDesc *obj);

    void cl_init_acquire(JavaThread *thread, void *holder);
}


#endif