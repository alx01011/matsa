#ifndef JTSAN_RTL_HPP
#define JTSAN_RTL_HPP

#include "oops/method.hpp"
#include "utilities/globalDefinitions.hpp"

#include "shadow.hpp"
#include "stacktrace.hpp"

#include <cstdint>

typedef uintptr_t uptr;

namespace JtsanRTL {
    void MemoryAccess(void *addr, Method *m, address &bcp, uint8_t access_size, bool is_write);
    bool CheckRaces(JavaThread *thread, JTSanStackTrace *trace, void *addr, ShadowCell &cur, ShadowCell &prev);
}

#endif

