#ifndef MATSA_INTERFACE_C1_HPP
#define MATSA_INTERFACE_C1_HPP

#include "oops/method.hpp"
#include "runtime/thread.hpp"
#include "utilities/globalDefinitions.hpp"

#define MATSA_MEMORY_ACCESS_C1(addr, offset, method, bci, size, type)   \
    void matsa_##type##_##size(void *addr, int offset, int bci, Method *method)

#define MATSA_ARRAY_ACCESS_C1(addr, idx, method, bci, size, type) \
    void matsa_array_##type##_##size(void *addr, int idx, BasicType array_type, int bci, Method *method)


namespace MaTSaC1 {
    MATSA_MEMORY_ACCESS_C1(addr, offset, method, bci, 1, read);
    MATSA_MEMORY_ACCESS_C1(addr, offset, method, bci, 1, write);

    MATSA_MEMORY_ACCESS_C1(addr, offset, method, bci, 2, read);
    MATSA_MEMORY_ACCESS_C1(addr, offset, method, bci, 2, write);

    MATSA_MEMORY_ACCESS_C1(addr, offset, method, bci, 4, read);
    MATSA_MEMORY_ACCESS_C1(addr, offset, method, bci, 4, write);

    MATSA_MEMORY_ACCESS_C1(addr, offset, method, bci, 8, read);
    MATSA_MEMORY_ACCESS_C1(addr, offset, method, bci, 8, write);

    extern void (*matsa_memory_access[2][9])(void *addr, int offset, int bci, Method *method);

    MATSA_ARRAY_ACCESS_C1(addr, idx, method, bci, 1, read);
    MATSA_ARRAY_ACCESS_C1(addr, idx, method, bci, 1, write);

    MATSA_ARRAY_ACCESS_C1(addr, idx, method, bci, 2, read);
    MATSA_ARRAY_ACCESS_C1(addr, idx, method, bci, 2, write);

    MATSA_ARRAY_ACCESS_C1(addr, idx, method, bci, 4, read);
    MATSA_ARRAY_ACCESS_C1(addr, idx, method, bci, 4, write);

    MATSA_ARRAY_ACCESS_C1(addr, idx, method, bci, 8, read);
    MATSA_ARRAY_ACCESS_C1(addr, idx, method, bci, 8, write);

    extern void (*matsa_array_access[2][9])(void *addr, int idx, BasicType array_type, int bci, Method *method);

    void method_enter(JavaThread *thread, Method *method);
    void method_exit(JavaThread *thread);

    void pre_method_enter(JavaThread *current, Method *method, int bci);

    void sync_enter(JavaThread *thread, oop obj);
    void sync_exit(JavaThread *thread, oop obj);

    void unlock(JavaThread *thread, void *obj);
}

#undef MATSA_MEMORY_ACCESS_C1
#undef MATSA_ARRAY_ACCESS_C1

#endif