#include "jtsanStack.hpp"

JTSanStack::JTSanStack(size_t size) {
    this->_stack = NEW_C_HEAP_ARRAY(uint64_t, size, mtInternal);
    this->top = 0;
    this->size = size;
}

JTSanStack::~JTSanStack() {
    FREE_C_HEAP_ARRAY(uint64_t, this->_stack);
}

void JTSanStack::push(uint64_t value) {
    // if we are at the end of the stack, double the size
    if (this->top == this->size) {
        this->size *= 2;
        uint64_t *new_stack = new uint64_t[this->size];

        for (size_t i = 0; i < this->top; i++) {
            new_stack[i] = this->_stack[i];
        }

        delete[] this->_stack;
        this->_stack = new_stack;
    }

    this->_stack[this->top++] = value;
}

uint64_t JTSanStack::pop(void) {
    return this->_stack[--this->top];
}