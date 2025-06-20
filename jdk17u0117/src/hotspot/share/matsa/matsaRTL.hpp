#ifndef MATSA_RTL_HPP
#define MATSA_RTL_HPP

#include "oops/method.hpp"
#include "utilities/globalDefinitions.hpp"
#include "runtime/basicLock.hpp"

#include "shadow.hpp"

#include <cstdint>

typedef uintptr_t uptr;

namespace MaTSaRTL {
    void InterpreterMemoryAccess(void *addr, Method *m, address bcp, uint8_t access_size, bool is_write);
    void C1MemoryAccess(void *addr, Method *m, int bci, uint8_t access_size, bool is_write);
    void C2MemoryAccess(void *addr, Method *m, int bci, uint8_t access_size, bool is_write);
    bool CheckRaces(void *addr, int32_t bci, ShadowCell &cur, ShadowCell &prev, HistoryCell &prev_history);
    bool FastCheckRaces(void *addr, int32_t bci, ShadowCell &cur, ShadowCell &prev, HistoryCell &prev_history);
};

#endif

