#ifndef JTSAN_DEFS_HPP
#define JTSAN_DEFS_HPP

// 256 threads
#define MAX_THREADS (1 << 8)

// TODO: this has to be done at compile time
// because we have to check if avx is supported
#define JTSAN_VECTORIZE 0
#if JTSAN_VECTORIZE
#include <immintrin.h>
typedef __m256i m256;
#endif

// branch prediction

#define LIKELY(x)   __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)

#endif