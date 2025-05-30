#ifndef MATSA_DEFS_HPP
#define MATSA_DEFS_HPP

// 256 threads
#define MAX_THREADS (1 << 8)


#define MAX_ADDRESS_BITS (48)
#define MAX_BCI_BITS     (16) // per spec
#define MAX_STACK_BITS   (18)


// TODO: this has to be done at compile time
// because we have to check if avx is supported
#ifdef MATSA_VECTORIZE
#undef MATSA_VECTORIZE
#define MATSA_VECTORIZE 0
#else
#define MATSA_VECTORIZE 0
#endif

#define GIB(x) ((x) * 1024ull * 1024ull * 1024ull)
#define TIB(x) ((x) * 1024ull * 1024ull * 1024ull * 1024ull)


// #if MATSA_VECTORIZE
// #undef MATSA_VECTORIZE
// #define MATSA_VECTORIZE __SSE4_2__
// #endif

#if MATSA_VECTORIZE
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
