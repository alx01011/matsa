#include "matsaStack.hpp"

MaTSaStack::MaTSaStack(size_t size) {
    this->_stack = NEW_C_HEAP_ARRAY(MaTSaStackElem, size, mtInternal);
    this->_top = -1;
    this->_caller_bci = 0; // fine for the initial call from main
    this->_size = size;
}

MaTSaStack::~MaTSaStack() {
    FREE_C_HEAP_ARRAY(MaTSaStackElem, this->_stack);
}

void MaTSaStack::push(Method *m, uint16_t bci) {
    assert(this->_top < this->_size - 1, "MaTSaStack overflow");
    assert(m != nullptr, "Method pointer cannot be null");
    assert((uint64_t)m != 0xf1f1f1f1f1f1, "Method pointer cannot be a magic value");

    MaTSaStackElem value = {(uint64_t)m, (uint64_t)bci};
    this->_stack[++this->_top] = value;
}

MaTSaStackElem MaTSaStack::pop(void) {
    assert(this->_top >= 0, "MaTSaStack underflow");
    return this->_stack[this->_top--];
}

MaTSaStackElem MaTSaStack::top(void) {
    assert(this->_top >= 0, "MaTSaStack underflow");
    return this->_stack[this->_top];
}

MaTSaStackElem MaTSaStack::get(int index) {
    assert(this->_top >= 0, "MaTSaStack underflow");
    assert(index >= 0, "Index cannot be negative");
    assert(index <= this->_top, "Index out of bounds");
    
    return this->_stack[index];
}

int MaTSaStack::size(void) {
    return this->_top + 1; // _top is the index of the last element
}

MaTSaStackElem *MaTSaStack::get(void) {
    return this->_stack;
}