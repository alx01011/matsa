#include "jtsanRTL.hpp"
#include "threadState.hpp"
#include "jtsanGlobals.hpp"
#include "jtsanReportMap.hpp"
#include "stacktrace.hpp"
#include "suppression.hpp"

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

bool JtsanRTL::CheckRaces(JavaThread *thread, JTSanStackTrace &trace, void *addr, ShadowCell &cur, ShadowCell &prev) {
    uptr addr_aligned = ((uptr)addr);

    bool stored   = false;
    bool isRace   = false;

    for (uint8_t i = 0; i < SHADOW_CELLS; i++) {
        ShadowCell cell = ShadowBlock::load_cell(addr_aligned, i);

       // if the cell is ignored then we can skip the whole block
        if (UNLIKELY(cell.is_ignored)) {
          return false;
        }

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

            // its a race, so check if it is a suppressed one
            JTSanStackTrace stack_trace(thread);
            trace = stack_trace;
            if (JTSanSuppression::is_suppressed(&stack_trace)) {
                // ignore
                cur.is_ignored = 1;
                ShadowBlock::store_cell_at((uptr)addr, &cur, 0);
                return false;
            }

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
    ShadowCell cur = {tid, epoch, (uint8_t)((uptr)addr & (8 - 1)), get_gc_epoch(), is_write, 0};

    // race
    ShadowCell prev;
    // try to lock the report lock
    JTSanStackTrace stack_trace(nullptr);
    if (CheckRaces(thread, stack_trace, addr, cur, prev) && !JTSanSilent && ShadowMemory::try_lock_report()) {
      // we have found a race now see if we have recently reported it or it is suppressed
      if (JtsanReportMap::get_instance()->get(addr) != nullptr) {
        // ignore
        ShadowMemory::unlock_report();
        return;
      }

        ResourceMark rm;
        int lineno = m->line_number_from_bci(m->bci_from(bcp));
        fprintf(stderr, "Data race detected in method %s, line %d\n",
            m->external_name_as_fully_qualified(),lineno);
        fprintf(stderr, "\t\tPrevious access %s of size %u, by thread %lu, epoch %lu, offset %lu\n",
            prev.is_write ? "write" : "read", access_size, prev.tid, prev.epoch, prev.offset);
        fprintf(stderr, "\t\tCurrent access %s of size %u, by thread %lu, epoch %lu, offset %lu\n",
            cur.is_write ? "write" : "read", access_size, cur.tid, cur.epoch, cur.offset);

        fprintf(stderr, "\t\t==================Stack trace==================\n");

        
        for (size_t i = 0; i < stack_trace.frame_count(); i++) {
            JTSanStackFrame frame = stack_trace.get_frame(i);
            int lineno = frame.method->line_number_from_bci(frame.method->bci_from(frame.pc));
            fprintf(stderr, "\t\t\t%s : %d\n", frame.method->external_name_as_fully_qualified(), lineno);
        }

        fprintf(stderr, "\t\t===============================================\n");

        // store the addr in the report map
        JtsanReportMap::get_instance()->put(addr);

        // unlock report lock after printing the report
        // this is to avoid multiple reports for consecutive accesses
        ShadowMemory::unlock_report();
    }
}