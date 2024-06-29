#include "jtsanRTL.hpp"
#include "threadState.hpp"
#include "jtsanGlobals.hpp"
#include "stacktrace.hpp"
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

#if !JTSAN_VECTORIZE
bool JtsanRTL::CheckRaces(JavaThread *thread, JTSanStackTrace* &trace, void *addr, ShadowCell &cur, ShadowCell &prev) {
    uptr addr_aligned = ((uptr)addr);

    bool stored   = false;
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

        // if the cell is ignored then we can skip the whole block
        if (UNLIKELY(cell.is_ignored)) {
          return false;
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
            trace = new JTSanStackTrace(thread);
            if (JTSanSuppression::is_suppressed(trace)) {
                // ignore
                isRace = false;
            }

            cur.is_ignored = 1;
            ShadowBlock::store_cell_at((uptr)addr, &cur, 0);
            stored = true;


            break;
        }
    }

    if (!stored) {
    // store the shadow cell
      ShadowBlock::store_cell((uptr)addr, &cur);
    }

    return isRace;
}
#else // JTSAN_VECTORIZE
bool JtsanRTL::CheckRaces(JavaThread *thread, JTSanStackTrace* &trace, void *addr, ShadowCell &cur, ShadowCell &prev) {
    void *shadow_addr = ShadowBlock::mem_to_shadow((uptr)addr);

    bool isRace    = false;
    bool isStored  = false;

    // load the shadow cells
    m256 shadow_vec  = _mm256_loadu_si256((m256 *)shadow_addr);

    const m256 tid_mask        = _mm256_set1_epi64x(0xFF);
    const m256 offset_mask     = _mm256_set1_epi64x(0x7);
    const m256 epoch_mask      = _mm256_set1_epi64x(0xFFFFFFFF);
    const m256 gc_epoch_mask   = _mm256_set1_epi64x(0x7FFFF);
    const m256 is_write_mask   = _mm256_set1_epi64x(0x1);
    const m256 is_ignored_mask = _mm256_set1_epi64x(0x1);

    m256 tid_vec               = _mm256_and_si256(shadow_vec, tid_mask);
    m256 epoch_vec             = _mm256_and_si256(shadow_vec, epoch_mask);
    m256 offset_vec            = _mm256_and_si256(shadow_vec, offset_mask);
    m256 gc_epoch_vec          = _mm256_and_si256(shadow_vec, gc_epoch_mask);
    m256 is_write_vec          = _mm256_and_si256(shadow_vec, is_write_mask);
    m256 is_ignored_vec        = _mm256_and_si256(shadow_vec, is_ignored_mask);

    /*
        If any of the cells in shadow is marked ignored, then we can skip the whole block
    */
    if (_mm256_movemask_epi8(is_ignored_vec) != 0) {
        return false;
    }

    /*
        Extract cells that meet the conditions:
        tid == cur.tid or both reads or cur.offset != offset or gc_epoch != cur.gc_epoch
    */
    m256 safe_cells = _mm256_cmpeq(tid_vec, _mm256_set1_epi64x(cur.tid));
         safe_cells = _mm256_or_si256(safe_cells, _mm256_or_si256(is_write_vec, _mm256_set1_epi64x(cur.is_write)));
         safe_cells = _mm256_or_si256(safe_cells, _mm256_or_si256(_mm256_cmpgt_epi64(offset_vec, _mm256_set1_epi64x(cur.offset)),
                                                  _mm256_cmpgt_epi64(_mm256_set1_epi64x(cur.offset), offset_vec)));
         safe_cells = _mm256_or_si256(safe_cells, _mm256_or_si256(_mm256_cmpgt_epi64(gc_epoch_vec, _mm256_set1_epi64x(cur.gc_epoch)),
                                                  _mm256_cmpgt_epi64(_mm256_set1_epi64x(cur.gc_epoch), gc_epoch_vec)));

    // extract safe cells
    shadow_vec = _mm256_andnot_si256(safe_cells, shadow_vec);     

    m256 thread_epochs = _mm256_set1_epi64x(0);
#define LOAD_EPOCH(i)\
    {\
        uint8_t  tid   = (uint8_t)_mm256_extract_epi64(tid_vec, i);\
        uint32_t epoch = JtsanThreadState::getEpoch(cur.tid, cell.tid);\
        thread_epochs  = _mm256_insert_epi64(thread_epochs, epoch, i);\
    }

    LOAD_EPOCH(0)
    LOAD_EPOCH(1)
    LOAD_EPOCH(2)
    LOAD_EPOCH(3)
#undef LOAD_EPOCH

    const m256 zero = _mm256_set1_epi64x(0);

    // race check
    // if cell is not zero and current.epoch < thread_epoch then is a race
    const m256 cmp  = _mm256_cmpgt_epi64(epoch_vec, thread_epochs);
    const m256 race = _mm256_andnot_si256(_mm256_cmpeq(zero, shadow_vec), cmp);

    int race_mask = _mm256_movemask_epi8(race);

    if (isRace = race_mask != 0) {
        trace = new JTSanStackTrace(thread);
        if (JTSanSuppression::is_suppressed(trace)) {
            // ignore
            isRace = false;
        }
        cur.is_ignored = 1;
        ShadowBlock::store_cell_at((uptr)addr, &cur, 0);

        isStored = true;

        int index = __builtin_ffs(race_mask) / 8;

        prev.tid      = (uint8_t) _mm256_extract_epi64(tid_vec, index);
        prev.epoch    = (uint32_t)_mm256_extract_epi64(epoch_vec, index);
        prev.offset   = (uint8_t) _mm256_extract_epi64(offset_vec, index);
        prev.gc_epoch = (uint16_t)_mm256_extract_epi64(gc_epoch_vec, index);
        prev.is_write = (uint8_t) _mm256_extract_epi64(is_write_vec, index);
    }

    if (!isStored) {
        ShadowBlock::store_cell((uptr)addr, &cur);
    }

    return isRace;
}
#endif

void JtsanRTL::MemoryAccess(void *addr, Method *m, address &bcp, uint8_t access_size, bool is_write) {
    JavaThread *thread = JavaThread::current();
    uint16_t tid       = JavaThread::get_jtsan_tid(thread);
    
    uint32_t epoch = JtsanThreadState::getEpoch(tid, tid);
    // create a new shadow cell
    ShadowCell cur = {tid, epoch, (uint8_t)((uptr)addr & (8 - 1)), get_gc_epoch(), is_write, 0};
    // symbolize the access
    Symbolizer::Symbolize(ACCESS, addr, m->bci_from(bcp), tid);

    // race
    ShadowCell prev;
    JTSanStackTrace *stack_trace = nullptr;
    if (CheckRaces(thread, stack_trace, addr, cur, prev) && !JTSanSilent) {
        JTSanReport::do_report_race(stack_trace, addr, access_size, bcp, m, cur, prev);
    }
}