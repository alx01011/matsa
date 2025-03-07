#ifndef MATSA_RTL_HPP
#define MATSA_RTL_HPP

#include "oops/method.hpp"
#include "utilities/globalDefinitions.hpp"
#include "runtime/basicLock.hpp"

#include "shadow.hpp"

#include <cstdint>

typedef uintptr_t uptr;

namespace MaTSaRTL {
    void MemoryAccess(void *addr, Method *m, address bcp, uint8_t access_size, bool is_write);
    void C1MemoryAccess(void *addr, Method *m, int bci, uint8_t access_size, bool is_write);
    bool CheckRaces(void *addr, int32_t bci, ShadowCell &cur, ShadowCell &prev, HistoryCell &prev_history);
    void matsa_store_x(int, int, void*, Method*);
    void matsa_load_x(int, int, void*, Method*);

    void matsa_load_array(int, void*, Method*, BasicType, int);

    void matsa_sync_enter(BasicObjectLock *lock, Method *m);
    void matsa_sync_exit(BasicObjectLock *lock);

    void matsa_pre_method_enter(Method *m, int bci);

    void matsa_method_enter(Method *m);
    void matsa_method_exit(Method *m);
};

#endif

