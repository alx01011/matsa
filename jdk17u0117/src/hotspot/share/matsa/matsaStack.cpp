#include "matsaStack.hpp"

MaTSaStack::MaTSaStack(size_t size) {
    this->_stack = NEW_C_HEAP_ARRAY(uint64_t, size, mtInternal);
    this->_top = 0;
    this->_size = size;
}

MaTSaStack::~MaTSaStack() {
    FREE_C_HEAP_ARRAY(uint64_t, this->_stack);
}

void MaTSaStack::push(uint64_t value) {
    // if we are at the end of the stack, double the size
    // if (this->_top == this->_size) {
    //     this->_size *= 2;
    //     uint64_t *new_stack = NEW_C_HEAP_ARRAY(uint64_t, this->_size, mtInternal);

    //     for (int i = 0; i < this->_top; i++) {
    //         new_stack[i] = this->_stack[i];
    //     }

    //     FREE_C_HEAP_ARRAY(uint64_t, this->_stack);
    //     this->_stack = new_stack;
    // }

    this->_stack[this->_top++] = value;
}

uint64_t MaTSaStack::pop(void) {
    return this->_stack[--this->_top];
}

uint64_t MaTSaStack::top(void) {
    if (this->_top == 0) {
        return 0;
    }
    
    return this->_stack[this->_top - 1];
}

uint64_t MaTSaStack::get(size_t index) {
    return this->_stack[index];
}


int MaTSaStack::size(void) {
    return this->_top;
}