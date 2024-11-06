#ifndef MATSA_STACK_HPP
#define MATSA_STACK_HPP

// 65k entries
#define DEFAULT_STACK_SIZE (1 << 16)

#include <cstdint>

#include "memory/allocation.hpp"

class MaTSaStack : public CHeapObj<mtInternal> {
    private:
        uint64_t *_stack;
        int _size;
        uint16_t _top;

        // needed so we know the line number a method got called at
        uint16_t _caller_bci;

    public:
        MaTSaStack(size_t size);
        ~MaTSaStack();

        void push(uint64_t value);
        uint64_t pop(void);

        uint64_t top(void);
        uint64_t get(size_t index);

        uint64_t *get(void);

        uint16_t get_caller_bci(void) { return _caller_bci; }
        void set_caller_bci(uint16_t bci) { _caller_bci = bci; }

        int size(void);
};

#endif