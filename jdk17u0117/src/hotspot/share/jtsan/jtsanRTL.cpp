#include "jtsanRTL.hpp"
#include "threadState.hpp"
#include "jtsanGlobals.hpp"
#include "suppression.hpp"
#include "report.hpp"
#include "symbolizer.hpp"
#include "jtsanDefs.hpp"

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

bool JtsanRTL::CheckRaces(JavaThread *thread, void *addr, address bcp, ShadowCell &cur, ShadowCell &prev, 
                           ShadowPair &pair) {
    uptr addr_aligned = ((uptr)addr);

    bool stored   = false;
    bool isRace   = false;

    void *base_shadow = ShadowMemory::MemToShadow(addr_aligned);

    pair.prev_shadow = base_shadow;

    for (uint8_t i = 0; i < SHADOW_CELLS; i++) {
        ShadowCell cell  = ShadowBlock::load_cell(addr_aligned, i);
        pair.prev_shadow = (void*)((uptr)pair.prev_shadow + (i * sizeof(ShadowCell)));

        // empty cell
        if (LIKELY(cell.epoch == 0)) {
            // can store
            if (!stored) {
              ShadowBlock::store_cell_at((uptr)addr, &cur, i);
              pair.cur_shadow = pair.prev_shadow;
              stored          = true;
            }
            continue;
        }

        // different offset means different memory location in case of 1,2 or 4 byte access
        if (LIKELY(cell.offset != cur.offset)) {
            continue;
        }

        // different gc epoch means different memory location
        // so we can skip
        if (UNLIKELY(cur.gc_epoch != cell.gc_epoch)) {
            // we can replace the cell
            // because the memory location it points to might have been freed or moved
            ShadowBlock::store_cell_at((uptr)addr, &cur, i);
            pair.cur_shadow = pair.prev_shadow;
            stored          = true;
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
              ShadowBlock::store_cell_at((uptr)addr, &cur, i);
              pair.cur_shadow = pair.prev_shadow;
              stored          = true;
          }
          continue;
        }

        // if the access is not a write then we can skip
        if (LIKELY(!(cell.is_write || cur.is_write))) {
            continue;
        }

        // at least one of the accesses is a write
        uint32_t thr = JTSanThreadState::getEpoch(cur.tid, cell.tid);

        // if the current thread has a higher epoch then we can skip
        if (LIKELY(thr >= cell.epoch)) {
            continue;
        }

        prev = cell;
        isRace = true;

        // its a race, so check if it is a suppressed one
        if (LIKELY(JTSanSuppression::is_suppressed(thread, bcp))) {
            // ignore
            isRace = false;
        }

        // mark ignore flag and store at ith index
        // so we can skip if ever encountered again
        // its fine if we miss it, we also check for previously reported races in do_report
        cur.is_ignored = 1;
        ShadowBlock::store_cell_at((uptr)addr, &cur, i);
        pair.cur_shadow = base_shadow;
        stored = true;


        break;
    }

    if (UNLIKELY(!stored)) {
    // store the shadow cell
    void *store_addr = ShadowBlock::store_cell((uptr)addr, &cur);
    pair.cur_shadow = store_addr;
    }

    return isRace;
}

void JtsanRTL::MemoryAccess(void *addr, Method *m, address &bcp, uint8_t access_size, bool is_write) {
    JavaThread *thread = JavaThread::current();
    uint16_t tid       = JavaThread::get_jtsan_tid(thread);
    
    uint32_t epoch = JTSanThreadState::getEpoch(tid, tid);
    // create a new shadow cell
    ShadowCell cur = {tid, epoch, (uint8_t)((uptr)addr & (8 - 1)), get_gc_epoch(), is_write, 0};

    // race
    ShadowCell prev;
    ShadowPair pair = {nullptr, nullptr};

    bool is_race = CheckRaces(thread, addr, bcp, cur, prev, pair);

    // symbolize the access
    // 1 is read, 2 is write
    Symbolizer::Symbolize((Event)(is_write + 1), addr, m->bci_from(bcp), tid, (uint64_t)pair.cur_shadow);

    if (is_race && !JTSanSilent) {
        JTSanReport::do_report_race(thread, addr, access_size, bcp, m, cur, prev, pair);
    }
}