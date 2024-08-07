#ifndef JTSAN_STACK_HPP
#define JTSAN_STACK_HPP

// 65k entries
#define DEFAULT_STACK_SIZE (1 << 16)

#include <cstdint>

#include "memory/allocation.hpp"

class JTSanStack : public CHeapObj<mtInternal> {
    private:
        uint64_t *_stack;
        size_t top;
        size_t size;

    public:
        JTSanStack(size_t size);
        ~JTSanStack();

        void push(uint64_t value);
        uint64_t pop(void);

        uint64_t top(void);
        uint64_t get(size_t index);

        size_t size(void);
};

#endif