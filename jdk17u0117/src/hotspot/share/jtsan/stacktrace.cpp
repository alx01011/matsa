#include "stacktrace.hpp"


JTSanStackTrace::JTSanStackTrace(Thread *thread) {
    _thread = thread;
    _frame_count = 0;

    RegisterMap reg_map(thread->as_Java_thread(), false);
    frame fr = os::current_frame();

    // skips native jtsan api calls
    // checkraces, memoryaccess, jtsan_read/write$size
    for (size_t i = 0; i < 3; i++) {
        fr = fr.sender(&reg_map);
    }

    for (; _frame_count < MAX_FRAMES;) {
        if (fr.is_first_frame() || fr.pc() == NULL) {
            break;
        }

        if (!fr.is_native_frame() && fr.is_interpreted_frame()) {
            Method *bt_method = fr.interpreter_frame_method();
            address bt_bcp    = fr.interpreter_frame_bcp();

            _frames[_frame_count].method = bt_method;
            _frames[_frame_count].pc = bt_bcp;
            _frame_count++;
        }

        fr = fr.sender(&reg_map);
    }
}

size_t JTSanStackTrace::frame_count(void) const {
    return _frame_count;
}

JTSanStackFrame JTSanStackTrace::get_frame(size_t index) const {
    return _frames[index];
}
