#ifndef JTSAN_DEFS_HPP
#define JTSAN_DEFS_HPP

// 256 threads
#define MAX_THREADS (1 << 8)

/*
    Fallback to 38 bits if not defined.
    This allows for heaps up to 2^38 bytes -> 256GiB
    We can compress the address to 38 bits and restore it later
    since the heap is contiguous and we know the base address its easy to restore
*/
#ifndef COMPRESSED_ADDR_BITS
#define COMPRESSED_ADDR_BITS 38
#endif

// TODO: this has to be done at compile time
// because we have to check if avx is supported
#ifndef JTSAN_VECTORIZE
#define JTSAN_VECTORIZE 0
#endif


// For now vectorization performs worse than scalar
// so we disable it
// #ifdef JTSAN_VECTORIZE
// #undef JTSAN_VECTORIZE
// #define JTSAN_VECTORIZE 0
// #endif

#if JTSAN_VECTORIZE
#include <emmintrin.h>
#include <smmintrin.h>
typedef __m256i m256;
typedef __m128i m128;
#endif

// branch prediction

#define LIKELY(x)   __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)

#endif