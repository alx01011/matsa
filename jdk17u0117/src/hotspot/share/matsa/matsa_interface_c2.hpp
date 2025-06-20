#ifndef MATSA_INTERFACE_C2_HPP
#define MATSA_INTERFACE_C2_HPP

#include "oops/method.hpp"
#include "runtime/thread.hpp"
#include "utilities/globalDefinitions.hpp"

#define MATSA_MEMORY_ACCESS_C2(addr, mbci_pack, size, type)   \
    void matsa_##type##_##size(void *addr, uint64_t mbci_pack)

namespace MaTSaC2 {
    MATSA_MEMORY_ACCESS_C2(addr, mbci_pack, 1, read);
    MATSA_MEMORY_ACCESS_C2(addr, mbci_pack, 1, write);

    MATSA_MEMORY_ACCESS_C2(addr, mbci_pack, 2, read);
    MATSA_MEMORY_ACCESS_C2(addr, mbci_pack, 2, write);

    MATSA_MEMORY_ACCESS_C2(addr, mbci_pack, 4, read);
    MATSA_MEMORY_ACCESS_C2(addr, mbci_pack, 4, write);

    MATSA_MEMORY_ACCESS_C2(addr, mbci_pack, 8, read);
    MATSA_MEMORY_ACCESS_C2(addr, mbci_pack, 8, write);

    extern void (*matsa_memory_access[2][9])(void *addr, uint64_t mbci_pack);

    void method_enter(JavaThread *thread, uint64_t mbci_pack);
    void method_exit(JavaThread *thread, uint64_t mbci_pack);

    void pre_method_enter(JavaThread *current, int bci);

    void sync_enter(JavaThread *thread, oopDesc *obj);
    void sync_exit(JavaThread *thread, oopDesc *obj);

    void cl_init_acquire(JavaThread *thread, void *holder);
}

#undef MATSA_MEMORY_ACCESS_C2


#endif