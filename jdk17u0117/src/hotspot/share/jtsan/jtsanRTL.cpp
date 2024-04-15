#include "jtsanRTL.hpp"
#include "threadState.hpp"
#include "jtsanGlobals.hpp"

#include "runtime/thread.hpp"
#include "memory/resourceArea.hpp"
#include "oops/oop.hpp"
#include "oops/oopsHierarchy.hpp"
#include "oops/oop.inline.hpp"

bool CheckRaces(uint16_t tid, void *addr, ShadowCell &cur) {
    for (uint8_t i = 0; i < SHADOW_CELLS; i++) {
        ShadowCell cell = ShadowBlock::load_cell((uptr)addr, i);

        // we can safely ignore if epoch is 0 it means cell is unassigned
        // or if the thread id is the same as the current thread 
        // previous access was by the same thread so we can skip
        if (cell.epoch == 0 || cell.tid == cur.tid) {
            continue;
        }

        // at least one of the accesses is a write
        if (cell.is_write + cur.is_write >= 1) {
            uint32_t thr = JtsanThreadState::getEpoch(tid, cell.tid);
            if (thr >= cell.epoch) {
                continue;
            }
           // if (print)
            //    fprintf(stderr, "Previous cell: index %d, tid %d, epoch %lu, is_write %d\n", i, cell.tid, cell.epoch, cell.is_write);
            //fprintf(stderr, "Previous access by thread %d, current thread %d\n", cell.tid, cur.tid);
            //fprintf(stderr, "Previous epoch %lu, current epoch %lu\n", cell.epoch, cur.epoch);
            return true;
        }
    }
    return false;
}


void MemoryAccess(void *addr, Method *m, address &bcp, uint8_t access_size, uint8_t is_write) {
    // if jtsan is not initialized, we can ignore
    if (!is_jtsan_initialized()) {
        return;
    }


    JavaThread *thread = JavaThread::current();
    uint16_t tid       = JavaThread::get_thread_obj_id(JavaThread::current());


    // increment the epoch of the current thread
    JtsanThreadState::incrementEpoch(tid);
    uint32_t epoch = JtsanThreadState::getEpoch(tid, tid);

    // create a new shadow cell
    ShadowCell cur = {tid, epoch, 0, is_write};

    // FIXME: this is slow
    oop obj = (oopDesc *)addr;
    // if (oopDesc::is_oop(obj) && obj->obj_lock_index()) {
    //     fprintf(stderr, "ignoring oop because of set index to %d\n", obj->obj_lock_index());
    //     return; // we can ignore locks
    // }

    // ResourceMark rm;

    // InstanceKlass *klass = m->method_holder();
    // file name
    // const char * fname = m->external_name_as_fully_qualified();

    // bool print = false;

    // if (strstr(fname, "Race")) {
    //     fprintf(stderr, "Access at line %d, in method %s\n", m->line_number_from_bci(m->bci_from(bcp)), m->external_name_as_fully_qualified());
    //     fprintf(stderr, "Current cell: tid %d, epoch %lu, is_write %d and epoch %u\n", cur.tid, cur.epoch, cur.is_write, epoch);
    //     print = true;
    // }

    // race
    if (CheckRaces(tid, addr, cur)) {
        ResourceMark rm;
        fprintf(stderr, "Data race detected in method %s, line %d\n",
            m->external_name_as_fully_qualified(), m->line_number_from_bci(m->bci_from(bcp)));
    }

    // store the shadow cell
    ShadowBlock::store_cell((uptr)addr, &cur);
}