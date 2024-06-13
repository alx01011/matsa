#include "jtsanRTL.hpp"
#include "threadState.hpp"
#include "jtsanGlobals.hpp"
#include "jtsanReportMap.hpp"

#include "runtime/thread.hpp"
#include "runtime/frame.inline.hpp"
#include "runtime/osThread.hpp"
#include "interpreter/interpreter.hpp"
#include "runtime/registerMap.hpp"
#include "memory/resourceArea.hpp"
#include "oops/oop.hpp"
#include "oops/oopsHierarchy.hpp"
#include "oops/oop.inline.hpp"
#include "utilities/decoder.hpp"

#define MAX_FRAMES (25)

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

bool JtsanRTL::CheckRaces(uint16_t tid, void *addr, ShadowCell &cur, ShadowCell &prev) {
    uptr addr_aligned = ((uptr)addr);

    bool stored = false;
    bool isRace   = false;

    for (uint8_t i = 0; i < SHADOW_CELLS; i++) {
        ShadowCell cell = ShadowBlock::load_cell(addr_aligned, i);
        // we can safely ignore if epoch is 0 it means cell is unassigned
        // or if the thread id is the same as the current thread 
        // previous access was by the same thread so we can skip
        // different offset means different memory location in case of 1,2 or 4 byte access
        if (cell.epoch == 0 || cur.gc_epoch != cell.gc_epoch || cell.offset != cur.offset) {
            continue;
        }

        // same slot this is not a race
        // even if a tid was reused it can't be race because the previous thread has obviously finished
        // making the tid available
        if (cell.tid == cur.tid) {
          // if the access is stronger overwrite
          if (cur.is_write && !cell.is_write) {
              ShadowBlock::store_cell_at((uptr)addr, &cur, i);
              stored = true;
          }
          continue;
        }

        // at least one of the accesses is a write
        if (cell.is_write || cur.is_write) {
            uint32_t thr = JtsanThreadState::getEpoch(cur.tid, cell.tid);

            if (thr >= cell.epoch) {
                continue;
            }
    
            prev = cell;
            isRace = true;
            break;
        }
    }

    if (!stored) {
    // store the shadow cell
      ShadowBlock::store_cell((uptr)addr, &cur);
    }

    return isRace;
}

void JtsanRTL::MemoryAccess(void *addr, Method *m, address &bcp, uint8_t access_size, bool is_write) {
    JavaThread *thread = JavaThread::current();
    uint16_t tid       = JavaThread::get_jtsan_tid(thread);
    
    uint32_t epoch = JtsanThreadState::getEpoch(tid, tid);
    // create a new shadow cell
    ShadowCell cur = {tid, epoch, (uint8_t)((uptr)addr & (8 - 1)), get_gc_epoch(), is_write};

    // race
    ShadowCell prev;
    // try to lock the report lock
    if (CheckRaces(tid, addr, cur, prev) && ShadowMemory::try_lock_report()) {
      // we have found a race now see if we have recently reported it
      if (JtsanReportMap::get_instance()->get(addr) != nullptr) {
        // ignore
        ShadowMemory::unlock_report();
        return;
      }

        ResourceMark rm;
        int lineno = m->line_number_from_bci(m->bci_from(bcp));
        fprintf(stderr, "Data race detected in method %s, line %d\n",
            m->external_name_as_fully_qualified(),lineno);
        fprintf(stderr, "\t\tPrevious access %s of size %d, by thread %d, epoch %lu, offset %d\n",
            prev.is_write ? "write" : "read", access_size, prev.tid, prev.epoch, prev.offset);
        fprintf(stderr, "\t\tCurrent access %s of size %d, by thread %d, epoch %lu, offset %d\n",
            cur.is_write ? "write" : "read", access_size, cur.tid, cur.epoch, cur.offset);

        fprintf(stderr, "\t\t==================Stack trace==================\n");

        frame fr = os::current_frame();
        // ignore the first frame as it is the current frame and we have already printed it
        fr = next_frame(fr, (Thread*)thread);
        // print the stack trace
        for (; fr.pc() != nullptr;) {
            if (Interpreter::contains(fr.pc())) {
                Method *bt_method = fr.interpreter_frame_method();
                address bt_bcp = (fr.is_interpreted_frame()) ? fr.interpreter_frame_bcp() : fr.pc();

                int lineno = bt_method->line_number_from_bci(bt_method->bci_from(bt_bcp));
                fprintf(stderr, "\t\t\t%s : %d\n", bt_method->external_name_as_fully_qualified(), lineno);
            }
            fr = next_frame(fr, (Thread*)thread);
        }

        fprintf(stderr, "\t\t===============================================\n");

        // store the bcp in the report map
        JtsanReportMap::get_instance()->put(addr);

        // unlock report lock after printing the report
        // this is to avoid multiple reports for consecutive accesses
        ShadowMemory::unlock_report();
    }
}