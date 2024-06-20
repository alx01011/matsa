#ifndef STACKTRACE_HPP
#define STACKTRACE_HPP

#include "interpreter/interpreter.hpp"
#include "memory/allocation.hpp"
#include "runtime/frame.inline.hpp"
#include "runtime/thread.hpp"
#include "runtime/osThread.hpp"

#include <cstddef>

// 256 frames maximum
#define MAX_FRAMES (1 << 8)

class JTSanStackFrame : public StackObj {
    public:
        Method *method;
        address pc;
};

class JTSanStackTrace : public CHeapObj<mtInternal> {
    public:
        JTSanStackTrace(Thread *thread);

        size_t          frame_count(void) const;
        JTSanStackFrame get_frame(size_t index) const;

    private:
        Thread *_thread;
        JTSanStackFrame _frames[MAX_FRAMES];
        size_t _frame_count;

};

#endif