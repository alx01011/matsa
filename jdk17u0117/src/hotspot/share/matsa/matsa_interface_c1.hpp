#ifndef MATSA_INTERFACE_C1_HPP
#define MATSA_INTERFACE_C1_HPP

#include "oops/method.hpp"
#include "utilities/globalDefinitions.hpp"

// matsa_typesize_c1
#define MATSA_MEMORY_ACCESS_C1(addr, offset, method, bci, size, type)   \
    void matsa_##type##_##size(void *addr, int offset, int bci, Method *method)

// void matsa_array_load/store_size(void *addr, int idx, int bci, Method *method)
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

    MATSA_ARRAY_ACCESS_C1(addr, idx, method, bci, 1, load);
    MATSA_ARRAY_ACCESS_C1(addr, idx, method, bci, 1, store);

    MATSA_ARRAY_ACCESS_C1(addr, idx, method, bci, 2, load);
    MATSA_ARRAY_ACCESS_C1(addr, idx, method, bci, 2, store);

    MATSA_ARRAY_ACCESS_C1(addr, idx, method, bci, 4, load);
    MATSA_ARRAY_ACCESS_C1(addr, idx, method, bci, 4, store);

    MATSA_ARRAY_ACCESS_C1(addr, idx, method, bci, 8, load);
    MATSA_ARRAY_ACCESS_C1(addr, idx, method, bci, 8, store);
}

#undef MATSA_MEMORY_ACCESS_C1
#undef MATSA_ARRAY_ACCESS_C1

#endif