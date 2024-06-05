#ifndef JTSAN_DEFS_HPP
#define JTSAN_DEFS_HPP

// 256 threads
#define MAX_THREADS (1 << 8)

// branch prediction

#define LIKELY(x)   __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)

#endif