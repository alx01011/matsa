#include "jtsanRTL.hpp"
#include "threadState.hpp"
#include "jtsanGlobals.hpp"

#include "runtime/thread.hpp"
#include "memory/resourceArea.hpp"
#include "oops/oop.hpp"
#include "oops/oopsHierarchy.hpp"
#include "oops/oop.inline.hpp"
#include "utilities/globalDefinitions.hpp"

bool CheckRaces(uint16_t tid, void *addr, ShadowCell &cur, ShadowCell &prev) {
    uptr addr_aligned = ((uptr)addr);

    for (uint8_t i = 0; i < SHADOW_CELLS; i++) {
        ShadowCell cell = ShadowBlock::load_cell(addr_aligned, i);
        // we can safely ignore if gc epoch is 0 it means cell is unassigned
        // or if the thread id is the same as the current thread 
        // previous access was by the same thread so we can skip
        // different offset means different memory location in case of 1,2 or 4 byte access
        if (cell.tid == cur.tid || cur.gc_epoch != cell.gc_epoch || cell.offset != cur.offset) {
            continue;
        }

        // at least one of the accesses is a write
        if (cell.is_write || cur.is_write) {
            uint32_t thr = JtsanThreadState::getEpoch(cur.tid, cell.tid);

            if (thr > cell.epoch) {
                continue;
            }
    
            prev = cell;
            return true;
        }
    }
    return false;
}


// FIXME: This will also report races on thread safe locks lke java.util.Concurrent.Lock
void MemoryAccess(void *addr, Method *m, address &bcp, uint8_t access_size, bool is_write, bool is_oop) {
    // if jtsan is not initialized, we can ignore
    if (!is_jtsan_initialized()) {
        return;
    }

    // ignore accesses from non-java threads
    if (!Thread::current()->is_Java_thread()) {
        return;
    }

    JavaThread *thread = JavaThread::current();
    uint16_t tid       = JavaThread::get_jtsan_tid(thread);
    
    uint32_t epoch = JtsanThreadState::getEpoch(tid, tid);
    // create a new shadow cell
    ShadowCell cur = {tid, epoch, (uint8_t)((uptr)addr & (8 - 1)), get_gc_epoch(), is_write};

    // race
    ShadowCell prev;
    if (CheckRaces(tid, addr, cur, prev)) {
        ResourceMark rm;
        fprintf(stderr, "Data race detected in method %s, line %d\n",
            m->external_name_as_fully_qualified(), m->line_number_from_bci(m->bci_from(bcp)));
        fprintf(stderr, "\t\tPrevious access %s of size %d, by thread %d, epoch %lu, offset %d\n",
            prev.is_write ? "write" : "read", access_size, prev.tid, prev.epoch, prev.offset);
        fprintf(stderr, "\t\tCurrent access %s of size %d, by thread %d, epoch %lu, offset %d\n",
            cur.is_write ? "write" : "read", access_size, cur.tid, cur.epoch, cur.offset);
    }

    // increment the epoch of the current thread
    JtsanThreadState::incrementEpoch(tid);
    // store the shadow cell
    ShadowBlock::store_cell((uptr)addr, &cur);
}