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
#ifdef JTSAN_VECTORIZE
#undef JTSAN_VECTORIZE
#define JTSAN_VECTORIZE 0
#else
#define JTSAN_VECTORIZE 0
#endif

#define GIB(x) ((x) * 1024ull * 1024ull * 1024ull)
#define TIB(x) ((x) * 1024ull * 1024ull * 1024ull * 1024ull)


// #if JTSAN_VECTORIZE
// #undef JTSAN_VECTORIZE
// #define JTSAN_VECTORIZE __SSE4_2__
// #endif

#if JTSAN_VECTORIZE
#include <emmintrin.h>
#include <smmintrin.h>
#include <immintrin.h>

typedef __m128i m128;
typedef __m256i m256;

#endif

// branch prediction

#ifdef __GNUC__
#define LIKELY(x)   __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)

#define ALIGNED(x) __attribute__((aligned(x)))

#else
#define LIKELY(x)   (x)
#define UNLIKELY(x) (x)
#define ALIGNED(x) 
#endif

#endif
