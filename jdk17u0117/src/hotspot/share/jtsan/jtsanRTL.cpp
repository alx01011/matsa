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

bool CheckRaces(uint16_t tid, MemAddr mem, ShadowCell &cur, ShadowCell &prev) {
    for (uint8_t i = 0; i < SHADOW_CELLS; i++) {
        ShadowCell cell = ShadowBlock::load_cell(mem, i);
        // we can safely ignore if gc epoch is 0 it means cell is unassigned
        // or if the thread id is the same as the current thread 
        // previous access was by the same thread so we can skip
        if (cell.tid == cur.tid || cur.gc_epoch != cell.gc_epoch) {
            continue;
        }

        // if (test) {
        //     fprintf(stderr, "\t i = %u, Previous access by thread %d at epoch %lu, gc_epoch %u, is_write %u\n",
        //         i, cell.tid, cell.epoch, cell.gc_epoch, cell.is_write);
        //     fprintf(stderr, "\t\t Epoch of cell.tid(%d) by cur.tid(%d) is %u\n", cell.tid, cur.tid, JtsanThreadState::getEpoch(cur.tid, cell.tid));
        // }

        // at least one of the accesses is a write
        if (cell.is_write || cur.is_write) {
            uint32_t thr = JtsanThreadState::getEpoch(cur.tid, cell.tid);

            if (thr > cell.epoch) {
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


// FIXME: This will also report races on thread safe locks lke java.util.Concurrent.Lock
void MemoryAccess(void *addr, Method *m, address &bcp, uint8_t access_size, bool is_write, bool is_oop) {
    // if jtsan is not initialized, we can ignore
    // we can also ignore during class initialization
    if (!is_jtsan_initialized()) {
        return;
    }

    // oop obj = (oopDesc *)addr;
    // // this means that the access is from a lock object
    // // we can ignore these
    // if (is_oop && oopDesc::is_oop(obj) && obj->obj_lock_index() != 0) {
    //     // find oop from address base
    //     fprintf(stderr, "Object is an oop but lock index is %d\n", obj->obj_lock_index());
    //     return;
    // }

    bool test = false;

    // ignore accesses from non-java threads
    if (!Thread::current()->is_Java_thread()) {
        return;
    }

    JavaThread *thread = JavaThread::current();
    uint16_t tid       = JavaThread::get_jtsan_tid(thread);

    // if (thread->is_thread_initializing()) {
    //     int lineno = m->line_number_from_bci(m->bci_from(bcp));
    //     if (lineno == 33) {
    //         fprintf(stderr, "skipping line 33 because thread %d is initializing\n", tid);
    //     }
    //     return; // ignore during init phase
    // }
    
    uint32_t epoch = JtsanThreadState::getEpoch(tid, tid);
    // create a new shadow cell
    ShadowCell cur = {tid, epoch, get_gc_epoch(), is_write};
    MemAddr    mem = {(uptr)addr, access_size};

    int lineno = m->line_number_from_bci(m->bci_from(bcp));
    if (lineno == 30 || lineno == 26) {
        // ShadowMemory *shadow = ShadowMemory::getInstance();

        // fprintf(stderr, "\t\tAccessing memory (%lu), at line %d by thread %d\n", (uptr)addr, lineno, tid);
        // fprintf(stderr, "\t\tShadow addr: %p\n", shadow->MemToShadow((uptr)addr));

    }

    // race
    ShadowCell prev;
    if (CheckRaces(tid, mem, cur, prev)) {
        ResourceMark rm;
        fprintf(stderr, "Data race detected in method %s, line %d\n",
            m->external_name_as_fully_qualified(), m->line_number_from_bci(m->bci_from(bcp)));
        fprintf(stderr, "Previous access %s of size %d, by thread %d, epoch %lu\n",
            prev.is_write ? "write" : "read", access_size, prev.tid, prev.epoch);
        fprintf(stderr, "Current access %s of size %d, by thread %d, epoch %lu\n",
            cur.is_write ? "write" : "read", access_size, cur.tid, cur.epoch);
    }

    // increment the epoch of the current thread
    JtsanThreadState::incrementEpoch(tid);
    // store the shadow cell
    ShadowBlock::store_cell(mem, &cur);
}