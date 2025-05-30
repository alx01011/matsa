#ifndef MATSA_STACK_HPP
#define MATSA_STACK_HPP

// 260k stack size by default
// should be enough for most cases
#define DEFAULT_STACK_SIZE (1 << 18)

#include <cstdint>

#include "memory/allocation.hpp"

class Method;

struct MaTSaStackElem {
    uint64_t method : 48; // method pointer
    uint64_t bci    : 16; // bci in the method
};

class MaTSaStack : public CHeapObj<mtInternal> {
    private:
        MaTSaStackElem *_stack;
        int _size;
        int _top;

        // needed so we know the line number a method got called at
        uint16_t _caller_bci;

    public:
        MaTSaStack(size_t size);
        ~MaTSaStack();

        void push(Method *m, uint16_t bci);
        MaTSaStackElem pop(void);

        MaTSaStackElem top(void);
        MaTSaStackElem get(int index);

        MaTSaStackElem *get(void);

        uint16_t get_caller_bci(void) { return _caller_bci; }
        void set_caller_bci(uint16_t bci) { _caller_bci = bci; }

        int size(void);
};

#endif