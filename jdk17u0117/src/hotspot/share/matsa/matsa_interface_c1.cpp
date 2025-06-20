#include "matsa_interface_c1.hpp"
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

#define MATSA_MEMORY_ACCESS_C1(addr, offset, method, bci, size, type, is_write)\
JRT_LEAF(void, MaTSaC1::matsa_##type##_##size(void *addr, int offset, int bci, Method *method))\
        void *true_addr = (void*)((uintptr_t)addr + offset);\
        MaTSaRTL::C1MemoryAccess(true_addr, method, bci, size, is_write);\
JRT_END
        


#define MATSA_ARRAY_ACCESS_C1(addr, idx, method, bci, size, type, is_write)\
JRT_LEAF(void, MaTSaC1::matsa_array_##type##_##size(void *addr, int idx, BasicType array_type, int bci, Method *method))\
        int offset_in_bytes = arrayOopDesc::base_offset_in_bytes(array_type);\
        int elem_size = type2aelembytes(array_type);\
        uint64_t disp = ((uint64_t)idx * elem_size) + offset_in_bytes;\
        void *true_addr = (void*)((uintptr_t)addr + disp);\
        MaTSaRTL::C1MemoryAccess(true_addr, method, bci, size, is_write);\
JRT_END

MATSA_MEMORY_ACCESS_C1(addr, offset, method, bci, 1, read, false);
MATSA_MEMORY_ACCESS_C1(addr, offset, method, bci, 2, read, false);
MATSA_MEMORY_ACCESS_C1(addr, offset, method, bci, 4, read, false);
MATSA_MEMORY_ACCESS_C1(addr, offset, method, bci, 8, read, false);

MATSA_MEMORY_ACCESS_C1(addr, offset, method, bci, 1, write, true);
MATSA_MEMORY_ACCESS_C1(addr, offset, method, bci, 2, write, true);
MATSA_MEMORY_ACCESS_C1(addr, offset, method, bci, 4, write, true);
MATSA_MEMORY_ACCESS_C1(addr, offset, method, bci, 8, write, true);


MATSA_ARRAY_ACCESS_C1(addr, idx, method, bci, 1, read, false);
MATSA_ARRAY_ACCESS_C1(addr, idx, method, bci, 2, read, false);
MATSA_ARRAY_ACCESS_C1(addr, idx, method, bci, 4, read, false);
MATSA_ARRAY_ACCESS_C1(addr, idx, method, bci, 8, read, false);

MATSA_ARRAY_ACCESS_C1(addr, idx, method, bci, 1, write, true);
MATSA_ARRAY_ACCESS_C1(addr, idx, method, bci, 2, write, true);
MATSA_ARRAY_ACCESS_C1(addr, idx, method, bci, 4, write, true);
MATSA_ARRAY_ACCESS_C1(addr, idx, method, bci, 8, write, true);


void (*MaTSaC1::matsa_memory_access[2][9])(void *addr, int offset, int bci, Method *method) = {
    {0, matsa_read_1, matsa_read_2, 0, matsa_read_4, 0, 0, 0, matsa_read_8},
    {0, matsa_write_1, matsa_write_2, 0, matsa_write_4, 0, 0, 0, matsa_write_8}
};

void (*MaTSaC1::matsa_array_access[2][9])(void *addr, int idx, BasicType array_type, int bci, Method *method) = {
    {0, matsa_array_read_1, matsa_array_read_2, 0, matsa_array_read_4, 0, 0, 0, matsa_array_read_8},
    {0, matsa_array_write_1, matsa_array_write_2, 0, matsa_array_write_4, 0, 0, 0, matsa_array_write_8}
};

JRT_LEAF(void, MaTSaC1::method_enter(JavaThread *thread, Method *method))
    int tid = JavaThread::get_matsa_tid(thread);

    MaTSaStack *stack = JavaThread::get_matsa_stack(thread);
    uint16_t bci = stack->get_caller_bci();

    stack->push(method, bci);
    History::add_event(thread, method, bci);
JRT_END

JRT_LEAF(void, MaTSaC1::method_exit(JavaThread *thread))
    int tid = JavaThread::get_matsa_tid(thread);
    MaTSaStack *stack = JavaThread::get_matsa_stack(thread);
    (void)stack->pop();
  
    History::add_event(thread, 0, 0);
JRT_END

JRT_LEAF(void, MaTSaC1::pre_method_enter(JavaThread *thread, Method *method, int bci))
    MaTSaStack *stack = JavaThread::get_matsa_stack(thread);
    // set the caller bci to be used by next func enter
    stack->set_caller_bci((uint16_t)bci);
JRT_END

JRT_LEAF(void, MaTSaC1::sync_enter(JavaThread *thread, BasicObjectLock *lock))
    int tid = JavaThread::get_matsa_tid(thread);  

    oop obj = lock->obj();
    
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

JRT_LEAF(void, MaTSaC1::sync_exit(JavaThread *thread, BasicObjectLock *lock))
    int tid = JavaThread::get_matsa_tid(thread);  

    oop obj = lock->obj();
    
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

JRT_LEAF(void, MaTSaC1::cl_init_acquire(JavaThread *thread, void *holder))
    int tid = JavaThread::get_matsa_tid(thread);

    oop lock_obj = (oopDesc*)holder;
    assert(oopDesc::is_oop(lock_obj), "must be a valid oop");

    LockShadow *obs = lock_obj->lock_state();
    Vectorclock* ts = obs->get_cl_init_vectorclock();

    Vectorclock* cur = MaTSaThreadState::getThreadState(tid);

    *cur = *ts;
JRT_END

#undef MATSA_MEMORY_ACCESS_C1
#undef MATSA_ARRAY_ACCESS_C1
