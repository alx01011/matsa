#ifndef JTSAN_RTL_HPP
#define JTSAN_RTL_HPP

#include "oops/method.hpp"
#include "utilities/globalDefinitions.hpp"

#include "shadow.hpp"

#include <cstdint>

typedef uintptr_t uptr;

void MemoryAccess(void *addr, Method *m, address &bcp, uint8_t access_size, bool is_write, bool is_oop = false);

bool CheckRaces(uint32_t *thr, void *addr, ShadowCell &cur, uint8_t is_write, ShadowCell &prev);

#endif

