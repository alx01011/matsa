#include "matsaRTL.hpp"
#include "threadState.hpp"
#include "matsaGlobals.hpp"
#include "suppression.hpp"
#include "report.hpp"
#include "matsaDefs.hpp"
#include "history.hpp"

#include <cstdio>

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

#include "runtime/interfaceSupport.inline.hpp"

bool MaTSaRTL::CheckRaces(void *addr, int32_t bci, ShadowCell &cur, ShadowCell &prev, HistoryCell &prev_history) {
    bool stored   = false;
    bool isRace   = false;

    HistoryCell cur_history = {(uint64_t)bci, History::get_part_idx(cur.tid),
                                 History::get_event_idx(cur.tid), History::get_cur_epoch(cur.tid)};

    for (uint8_t i = 0; i < SHADOW_CELLS; i++) {
        ShadowCell cell  = ShadowBlock::load_cell((uptr)addr, i);

        // empty cell
        if (LIKELY(cell.epoch == 0)) {
            // can store
            if (!stored) {
              ShadowBlock::store_cell_at((uptr)addr, &cur, &cur_history, i);
              stored          = true;
            }
            continue;
        }

        // different offset means different memory location in case of 1,2 or 4 byte access
        if (LIKELY(cell.offset != cur.offset)) {
            continue;
        }

        // if the cell is ignored then we can skip the whole block
        if (UNLIKELY(cell.is_ignored)) {
            return false;
        }

        // same slot this is not a race
        // even if a tid was reused it can't be race because the previous thread has obviously finished
        // making the tid available
        if (LIKELY(cell.tid == cur.tid)) {
          // if the access is stronger overwrite
          if (LIKELY(cur.is_write && !cell.is_write)) {
              ShadowBlock::store_cell_at((uptr)addr, &cur, &cur_history,i);
              stored = true;
          }
          continue;
        }

        // if the access is not a write then we can skip
        if (LIKELY(!(cell.is_write || cur.is_write))) {
            continue;
        }

        // at least one of the accesses is a write
        uint32_t thr = MaTSaThreadState::getEpoch(cur.tid, cell.tid);

        // if the current thread has a higher epoch then we can skip
        if (LIKELY(thr >= cell.epoch)) {
            continue;
        }

        prev = cell;
        prev_history = ShadowBlock::load_history((uptr)addr, i);
        isRace = true;

        // mark ignore flag and store at ith index
        // so we can skip if ever encountered again
        // its fine if we miss it, we also check for previously reported races in do_report
        cur.is_ignored = 1;
        ShadowBlock::store_cell_at((uptr)addr, &cur, &cur_history, i);
        stored = true;


        break;
    }

    if (UNLIKELY(!stored)) {
        // store the shadow cell
        ShadowBlock::store_cell((uptr)addr, &cur, &cur_history);
    }

    return isRace;
}

void MaTSaRTL::MemoryAccess(void *addr, Method *m, address &bcp, uint8_t access_size, bool is_write) {
    JavaThread *thread = JavaThread::current();
    uint16_t tid       = JavaThread::get_matsa_tid(thread);
    
    int32_t  bci   = m->bci_from(bcp);
    uint32_t epoch = MaTSaThreadState::getEpoch(tid, tid);
    // create a new shadow cell
    ShadowCell cur = {tid, epoch, (uint8_t)((uptr)addr & (8 - 1)), is_write, 0};

    int lineno = m->line_number_from_bci(bci);

    if (lineno == 38 || lineno == 39) {
        ResourceMark rm;
        const char *method_name = m->external_name_as_fully_qualified();
        //if (strstr(method_name, "InterThreadLatency") != NULL) {
            fprintf(stderr, "interpreter store %p, method: %s, line: %d\n", addr, method_name, lineno);
        //}
    }

    // race
    ShadowCell prev;
    HistoryCell prev_history;
    bool is_race = CheckRaces(addr, bci, cur, prev, prev_history);



    // symbolize the access
    // 1 is read, 2 is write
    //Symbolizer::Symbolize((Event)(is_write + 1), addr, m->bci_from(bcp), tid);

    if (is_race && !MaTSaSilent) {
        MaTSaReport::do_report_race(thread, addr, access_size, bcp, m, cur, prev, prev_history);
    }
}

JRT_LEAF(void, MaTSaRTL::matsa_store_x(int offset, int bci, void *addr, Method *m))
    ResourceMark rm;
    uintptr_t true_addr = (uintptr_t)addr + offset;
    int lineno = m->line_number_from_bci(bci);

    // const char *method_name = m->external_name_as_fully_qualified();
    // if (strstr(method_name, "InterThreadLatency") != NULL) {
    //     fprintf(stderr, "matsa_store %p(%p), method: %s, line: %d\n", (void*)true_addr, addr, method_name, lineno);

    // }

    return;
JRT_END

JRT_LEAF(void, MaTSaRTL::matsa_load_x(int *x))
    return;
JRT_END

JRT_LEAF(void, MaTSaRTL::matsa_sync_enter(BasicObjectLock *lock, Method *m))
    ResourceMark rm;

    oop obj = lock->obj();
    fprintf(stderr, "matsa_sync_enter: obj -> %s, method -> %s\n", 
        obj->klass()->external_name(), m->external_name_as_fully_qualified());

    return;
JRT_END

JRT_LEAF(void, MaTSaRTL::matsa_sync_exit(BasicObjectLock *lock))
    return;
JRT_END