#include "matsa_interface_c1.hpp"
#include "matsaRTL.hpp"

#include <cstdint>

#define MATSA_MEMORY_ACCESS_C1(addr, offset, method, bci, size, type)   \
    void MaTSaC1::matsa_##type##_##size(void *addr, int offset, int bci, Method *method) { \
        void *true_addr = (void*)((uintptr_t)addr + offset); \
        address bcp = method->bcp_from(bci); \
        MaTSaRTL::MemoryAccess(true_addr, method, bcp, size, type == write); \
    }

#define MATSA_ARRAY_ACCESS_C1(addr, idx, method, bci, size, type) \
    void MaTSaC1::matsa_array_##type##_##size(void *addr, int idx, BasicType array_type, int bci, Method *method) { \
        int offset_in_bytes = arrayOopDesc::base_offset_in_bytes(array_type); \
        int elem_size = type2aelembytes(array_type); \
        uint64_t disp = ((uint64_t)idx * elem_size) + offset_in_bytes; \
        void *true_address = (void*)((uintptr_t)addr + disp); \
        address bcp = method->bcp_from(bci); \

        MaTSaRTL::MemoryAccess(true_address, method, bcp, size, type == write); \
    }

MATSA_MEMORY_ACCESS_C1(addr, offset, method, bci, 1, read);
MATSA_MEMORY_ACCESS_C1(addr, offset, method, bci, 1, write);

MATSA_MEMORY_ACCESS_C1(addr, offset, method, bci, 2, read);
MATSA_MEMORY_ACCESS_C1(addr, offset, method, bci, 2, write);

MATSA_MEMORY_ACCESS_C1(addr, offset, method, bci, 4, read);
MATSA_MEMORY_ACCESS_C1(addr, offset, method, bci, 4, write);

MATSA_MEMORY_ACCESS_C1(addr, offset, method, bci, 8, read);
MATSA_MEMORY_ACCESS_C1(addr, offset, method, bci, 8, write);

MATSA_ARRAY_ACCESS_C1(addr, idx, method, bci, 1, read);
MATSA_ARRAY_ACCESS_C1(addr, idx, method, bci, 1, write);

MATSA_ARRAY_ACCESS_C1(addr, idx, method, bci, 2, read);
MATSA_ARRAY_ACCESS_C1(addr, idx, method, bci, 2, write);

MATSA_ARRAY_ACCESS_C1(addr, idx, method, bci, 4, read);
MATSA_ARRAY_ACCESS_C1(addr, idx, method, bci, 4, write);

MATSA_ARRAY_ACCESS_C1(addr, idx, method, bci, 8, read);
MATSA_ARRAY_ACCESS_C1(addr, idx, method, bci, 8, write);

#undef MATSA_MEMORY_ACCESS_C1
#undef MATSA_ARRAY_ACCESS_C1
