#include "matsa_interface_c2.hpp"
#include "matsaRTL.hpp"
#include "matsaStack.hpp"
#include "history.hpp"
#include "threadState.hpp"
#include "lockState.hpp"
#include "oops/oop.inline.hpp"

#include "utilities/globalDefinitions.hpp"
#include "runtime/interfaceSupport.inline.hpp"

#include "memory/resourceArea.hpp"

#include <cstdint>

#define MATSA_MEMORY_ACCESS_C2(size, type, is_write)\
JRT_LEAF(void, MaTSaC2::matsa_##type##_##size(void *addr, uint64_t mbci_pack))\
        Method *m = (Method*)(mbci_pack & (1ULL << 48) - 1); \
        uint16_t bci = (uint16_t)((mbci_pack >> 48));\
        MaTSaRTL::C2MemoryAccess(addr, m, (int)bci, size, is_write);\
JRT_END

#define MATSA_STATIC_MEMORY_ACCESS_C2(size, type, is_write)\
JRT_LEAF(void, MaTSaC2::matsa_static_##type##_##size(void *obj, void *addr, uint64_t mbci_pack))\
        MaTSaC2::cl_init_acquire(JavaThread::current(), obj);\
        Method *m = (Method*)(mbci_pack & (1ULL << 48) - 1); \
        uint16_t bci = (uint16_t)((mbci_pack >> 48));\
        MaTSaRTL::C2MemoryAccess(addr, m, (int)bci, size, is_write);\
JRT_END

MATSA_MEMORY_ACCESS_C2(1, read, false);
MATSA_MEMORY_ACCESS_C2(2, read, false);
MATSA_MEMORY_ACCESS_C2(4, read, false);
MATSA_MEMORY_ACCESS_C2(8, read, false);

MATSA_MEMORY_ACCESS_C2(1, write, true);
MATSA_MEMORY_ACCESS_C2(2, write, true);
MATSA_MEMORY_ACCESS_C2(4, write, true);
MATSA_MEMORY_ACCESS_C2(8, write, true);

#undef MATSA_MEMORY_ACCESS_C2

MATSA_STATIC_MEMORY_ACCESS_C2(1, read, false);
MATSA_STATIC_MEMORY_ACCESS_C2(2, read, false);
MATSA_STATIC_MEMORY_ACCESS_C2(4, read, false);
MATSA_STATIC_MEMORY_ACCESS_C2(8, read, false);

MATSA_STATIC_MEMORY_ACCESS_C2(1, write, true);
MATSA_STATIC_MEMORY_ACCESS_C2(2, write, true);
MATSA_STATIC_MEMORY_ACCESS_C2(4, write, true);
MATSA_STATIC_MEMORY_ACCESS_C2(8, write, true);

#undef MATSA_STATIC_MEMORY_ACCESS_C2


void (*MaTSaC2::matsa_memory_access[2][9])(void *addr, uint64_t mbci_pack) = {
    {0, matsa_read_1, matsa_read_2, 0, matsa_read_4, 0, 0, 0, matsa_read_8},
    {0, matsa_write_1, matsa_write_2, 0, matsa_write_4, 0, 0, 0, matsa_write_8}
};

void (*MaTSaC2::matsa_static_memory_access[2][9])(void *obj, void *addr, uint64_t mbci_pack) = {
    {0, matsa_static_read_1, matsa_static_read_2, 0, matsa_static_read_4, 0, 0, 0, matsa_static_read_8},
    {0, matsa_static_write_1, matsa_static_write_2, 0, matsa_static_write_4, 0, 0, 0, matsa_static_write_8}
};


void MaTSaC2::method_enter(JavaThread *thread, uint64_t mbci_pack) {
    MaTSaStack *stack = JavaThread::get_matsa_stack(thread);

    uint64_t mptr = mbci_pack & ((1ULL << 48) - 1);
    uint16_t bci = (uint16_t)(mbci_pack >> 48);

    Method *method = (Method*)mptr;

    // Symbolizer::Symbolize(FUNC, method, bci, tid);
    stack->push(method, bci);
    History::add_event(thread, method, (uint16_t)bci);
}

void MaTSaC2::method_exit(JavaThread *thread, uint64_t mbci_pack) {
    MaTSaStack *stack = JavaThread::get_matsa_stack(thread);
    (void)stack->pop();
  
    History::add_event(thread, 0, 0);
}

void MaTSaC2::pre_method_enter(JavaThread *thread, int bci) {
    MaTSaStack *stack = JavaThread::get_matsa_stack(thread);
    // set the caller bci to be used by next func enter
    stack->set_caller_bci((uint16_t)bci);
}

JRT_LEAF(void, MaTSaC2::sync_enter(JavaThread *thread, oopDesc *obj))
    int tid = JavaThread::get_matsa_tid(thread);
    
    assert(oopDesc::is_oop(obj), "must be a valid oop");

    /*
        On lock acquisition we have to perform a max operation between the thread state of current thread and the lock state.
        Store the result into the thread state.
    */

    LockShadow *sls = obj->lock_state();
    Vectorclock* ts = sls->get_vectorclock();

    Vectorclock* cur = MaTSaThreadState::getThreadState(tid);

    *cur = *ts;
JRT_END

JRT_LEAF(void, MaTSaC2::sync_exit(JavaThread *thread, oopDesc *obj))
    int tid = JavaThread::get_matsa_tid(thread);
    
    assert(oopDesc::is_oop(obj), "must be a valid oop");

    /*
      On lock release we have to max the thread state with the lock state.
      Store the result into lock state.
    */
  
    LockShadow* sls = obj->lock_state();
    Vectorclock* ls = sls->get_vectorclock();
    Vectorclock* cur = MaTSaThreadState::getThreadState(tid);
  
    *ls = *cur;
  
    // increment the epoch of the current thread after the transfer
    MaTSaThreadState::incrementEpoch(tid);
JRT_END

JRT_LEAF(void, MaTSaC2::cl_init_acquire(JavaThread *thread, void *holder))
    int tid = JavaThread::get_matsa_tid(thread);

    oop lock_obj = (oopDesc*)holder;
    assert(oopDesc::is_oop(lock_obj), "must be a valid oop");

    LockShadow *obs = lock_obj->lock_state();
    Vectorclock* ts = obs->get_cl_init_vectorclock();

    Vectorclock* cur = MaTSaThreadState::getThreadState(tid);

    *cur = *ts;
JRT_END