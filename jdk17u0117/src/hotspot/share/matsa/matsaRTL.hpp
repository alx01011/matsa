#ifndef MATSA_RTL_HPP
#define MATSA_RTL_HPP

#include "oops/method.hpp"
#include "utilities/globalDefinitions.hpp"

#include "shadow.hpp"

#include <cstdint>

typedef uintptr_t uptr;

namespace MaTSaRTL {
    void MemoryAccess(void *addr, Method *m, address &bcp, uint8_t access_size, bool is_write);
    bool CheckRaces(void *addr, int32_t bci, ShadowCell &cur, ShadowCell &prev, HistoryCell &prev_history);
};

#endif

