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


JRT_LEAF(void, MaTSaC2::method_enter(JavaThread *thread, Method *method))
    int tid = JavaThread::get_matsa_tid(thread);

    MaTSaStack *stack = JavaThread::get_matsa_stack(thread);
    uint16_t bci = stack->get_caller_bci();
    // first 48 bits are the method id, last 16 bits are the bci
    uint64_t packed_frame = ((uint64_t)method << 16) | (uint64_t)bci;

    // Symbolizer::Symbolize(FUNC, method, bci, tid);
    stack->push(packed_frame);
    History::add_event(thread, method, bci);
JRT_END

JRT_LEAF(void, MaTSaC2::method_exit(JavaThread *thread, Method *method))
    int tid = JavaThread::get_matsa_tid(thread);
    MaTSaStack *stack = JavaThread::get_matsa_stack(thread);
    (void)stack->pop();
  
    History::add_event(thread, 0, 0);
JRT_END

JRT_LEAF(void, MaTSaC2::pre_method_enter(JavaThread *thread, int bci))
    MaTSaStack *stack = JavaThread::get_matsa_stack(thread);
    // set the caller bci to be used by next func enter
    stack->set_caller_bci((uint16_t)bci);
JRT_END

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
