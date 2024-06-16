#include "stacktrace.hpp"

static frame next_frame(frame fr, Thread* t) {
  // Compiled code may use EBP register on x86 so it looks like
  // non-walkable C frame. Use frame.sender() for java frames.
  frame invalid;
  if (t != nullptr && t->is_Java_thread()) {
    // Catch very first native frame by using stack address.
    // For JavaThread stack_base and stack_size should be set.
    if (!t->is_in_full_stack((address)(fr.real_fp() + 1))) {
      return invalid;
    }
    if (fr.is_java_frame() || fr.is_native_frame() || fr.is_runtime_frame()) {
      RegisterMap map(t->as_Java_thread(), false); // No update
      return fr.sender(&map);
    } else {
      // is_first_C_frame() does only simple checks for frame pointer,
      // it will pass if java compiled code has a pointer in EBP.
      if (os::is_first_C_frame(&fr)) return invalid;
      return os::get_sender_for_C_frame(&fr);
    }
  } else {
    if (os::is_first_C_frame(&fr)) return invalid;
    return os::get_sender_for_C_frame(&fr);
  }
}

JTSanStackTrace::JTSanStackTrace(Thread *thread) {
    _thread = thread;
    _frame_count = 0;

    
    if (_thread != nullptr) {
        frame fr = os::current_frame();
        for (; fr.pc() != nullptr;) {
            if (Interpreter::contains(fr.pc())) {
                Method *bt_method = fr.interpreter_frame_method();
                address bt_bcp = (fr.is_interpreted_frame()) ? fr.interpreter_frame_bcp() : fr.pc();

                _frames[_frame_count].method = bt_method;
                _frames[_frame_count].pc = bt_bcp;
                _frame_count++;
            }
            fr = next_frame(fr, (Thread*)thread);
        }

    }
}

size_t JTSanStackTrace::frame_count(void) const {
    return _frame_count;
}

JTSanStackFrame JTSanStackTrace::get_frame(size_t index) const {
    return _frames[index];
}
