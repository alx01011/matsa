#include "jtsanRTL.hpp"
#include "threadState.hpp"
#include "jtsanGlobals.hpp"

#include "runtime/thread.hpp"
#include "memory/resourceArea.hpp"
#include "oops/oop.hpp"
#include "oops/oopsHierarchy.hpp"
#include "oops/oop.inline.hpp"
#include "utilities/globalDefinitions.hpp"

uint8_t TosToSize(TosState tos) {
    uint8_t lookup[] = 
    {1 /*byte*/, 1 /*byte*/, 1 /*char*/, 2 /*short*/, 
    4 /*int*/, 8 /*long*/,  4 /*float*/, 8 /*double*/, 
    8 /*object*/};

    // this is safe because we only enter this function if tos is one of above
    return lookup[tos];
}

bool CheckRaces(uint16_t tid, void *addr, ShadowCell &cur, ShadowCell &prev) {
    for (uint8_t i = 0; i < SHADOW_CELLS; i++) {
        ShadowCell cell = ShadowBlock::load_cell((uptr)addr, i);

        // we can safely ignore if epoch is 0 it means cell is unassigned
        // or if the thread id is the same as the current thread 
        // previous access was by the same thread so we can skip
        if (cell.epoch == 0 || cell.tid == cur.tid || cur.gc_epoch != cell.gc_epoch) {
            continue;
        }

        // at least one of the accesses is a write
        if (cell.is_write + cur.is_write >= 1) {
            uint32_t thr = JtsanThreadState::getEpoch(tid, cell.tid);
            if (thr >= cell.epoch) {
                continue;
            }
           // if (print)
            //fprintf(stderr, "Previous access by thread %d, current thread %d\n", cell.tid, cur.tid);
            //fprintf(stderr, "Previous epoch %lu, current epoch %lu\n", cell.epoch, cur.epoch);
            prev = cell;
            return true;
        }
    }
    return false;
}


void MemoryAccess(void *addr, Method *m, address &bcp, uint8_t access_size, bool is_write, bool is_oop = false) {
    // if jtsan is not initialized, we can ignore
    if (!is_jtsan_initialized()) {
        return;
    }

    // FIXME: this is slow
    oop obj = (oopDesc *)addr;
    // this means that the access is from a lock object
    // we can ignore these
    if (is_oop && oopDesc::is_oop(obj) && obj->obj_lock_index() != 0) {
        if (!oopDesc::is_oop(obj)) {
            fprintf(stderr, "Object is not an oop\n");
        } else {
            fprintf(stderr, "Object is an oop but lock index is %d\n", obj->obj_lock_index());
        }
        return;
    }

    JavaThread *thread = JavaThread::current();
    uint16_t tid       = JavaThread::get_thread_obj_id(JavaThread::current());

    // increment the epoch of the current thread
    JtsanThreadState::incrementEpoch(tid);
    uint32_t epoch = JtsanThreadState::getEpoch(tid, tid);

    // create a new shadow cell
    ShadowCell cur = {tid, epoch, get_gc_epoch(), is_write};

    // race
    ShadowCell prev;
    if (CheckRaces(tid, addr, cur, prev)) {
        ResourceMark rm;
        fprintf(stderr, "Data race detected in method %s, line %d\n",
            m->external_name_as_fully_qualified(), m->line_number_from_bci(m->bci_from(bcp)));
        fprintf(stderr, "Previous access %s of size %d, by thread %d, epoch %lu\n",
            prev.is_write ? "write" : "read", access_size, prev.tid, prev.epoch);
    }

    // store the shadow cell
    ShadowBlock::store_cell((uptr)addr, &cur);
}