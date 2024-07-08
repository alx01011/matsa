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


#if JTSAN_VECTORIZE
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
        if (LIKELY(cell.epoch == 0 || cur.gc_epoch != cell.gc_epoch || cell.offset != cur.offset)) {
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
              stored = true;
          }
          continue;
        }

        if (LIKELY(!(cell.is_write || cur.is_write))) {
            continue;
        }

        // at least one of the accesses is a write
        uint32_t thr = JTSanThreadState::getEpoch(cur.tid, cell.tid);

        if (LIKELY(thr >= cell.epoch)) {
            continue;
        }

        prev = cell;
        isRace = true;

        // its a race, so check if it is a suppressed one
        trace = new JTSanStackTrace(thread);
        if (LIKELY(JTSanSuppression::is_suppressed(trace))) {
            // ignore
            isRace = false;
        }

        cur.is_ignored = 1;
        ShadowBlock::store_cell_at((uptr)addr, &cur, 0);
        stored = true;


        break;
    }

    if (UNLIKELY(!stored)) {
    // store the shadow cell
      uint8_t index = JTSanThreadState::getHistory(cur.tid)->index.load(std::memory_order_relaxed) % SHADOW_CELLS;
      ShadowBlock::store_cell_at((uptr)addr, &cur, index);
    }

    return isRace;
}

#else

bool JtsanRTL::CheckRaces(JavaThread *thread, JTSanStackTrace* &trace, void *addr, ShadowCell &cur, ShadowCell &prev) {
    void *shadow_addr = ShadowMemory::MemToShadow((uptr)addr);

    m256 shadow        = _mm256_load_si256((__m256i*)shadow_addr);
    m256 thread_epochs = _mm256_setzero_si256();

    // load cur
    const m256 cur_cell = _mm256_insert_epi64(zero, *(uint64_t)&cur, 0);

    const m256 zero = _mm256_setzero_si256();
    const m256 one  = _mm256_set1_epi64x(1);

    const m256 tid        = _mm256_and_si256(shadow, _mm256_set1_epi64x(0xFF));
    const m256 offset     = _mm256_and_si256(_mm256_srli_epi64(shadow, 40), _mm256_set1_epi64x(0x7));
    const m256 gc_epoch   = _mm256_and_si256(_mm256_srli_epi64(shadow, 43), _mm256_set1_epi64x(0x7FFFF));
    const m256 is_write   = _mm256_and_si256(_mm256_srli_epi64(shadow, 62), _mm256_set1_epi64x(0x1));
    const m256 is_ignored = _mm256_and_si256(_mm256_srli_epi64(shadow, 63), _mm256_set1_epi64x(0x1));

    const m256 is_ignored = _mm256_cmpeq_epi64(is_ignored, one);
    // ignored bit set in one of the cells
    if (_mm256_movemask_epi8(is_ignored)) {
        return false;
    }

    const m256 epoch = _mm256_and_si256(_mm256_srli_epi64(shadow, 8), _mm256_set1_epi64x(0xFFFFFFFF));

          // tid == current.id
    m256 skip_mask = _mm256_cmpeq_epi64(tid, _mm256_set1_epi64x(cur.tid));
         // r/r
         skip_mask = _mm256_or_si256(skip_mask, _mm256_or_si256(is_write, _mm256_set1_epi64x(cur.is_write)));
         // offset != cur.offset
         skip_mask = _mm256_or_si256(skip_mask, _mm256_cmpeq_epi64(offset, _mm256_set1_epi64x(cur.offset)));
         // gc_epoch != cur.gc_epoch
         skip_mask = _mm256_or_si256(skip_mask, _mm256_cmpeq_epi64(gc_epoch, _mm256_set1_epi64x(cur.gc_epoch)));

        shadow = _mm256_andnot_si256(skip_mask, shadow);

#define LOAD_EPOCH(idx)\
    {\
        uint8_t tid    = _mm256_extract_epi8(tid, idx * 8);\
        uint32_t epoch = JTSanThreadState::getEpoch(cur.tid, tid);\
        thread_epochs = _mm256_insert_epi32(thread_epochs, epoch, idx);\
    }

    LOAD_EPOCH(0);
    LOAD_EPOCH(1);
    LOAD_EPOCH(2);
    LOAD_EPOCH(3);

#undef LOAD_EPOCH

    // thr >= cell.epoch
    const m256 cmp = _mm256_cmpgt_epi32(thread_epochs, epoch);
    // if thr >= cell.epoch then skip
    const m256 report_mask = _mm256_andnot_si256(_mm256_cmpeq_epi64(zero, shadow), cmp);

    int report = _mm256_movemask_epi8(report_mask);

    // race
    if (report) {
        int idx = __builtin_ffs(report) / 8;

        prev.tid      = _mm256_extract_epi8(tid, idx * 8);
        prev.epoch    = _mm256_extract_epi32(epoch, idx);
        prev.offset   = _mm256_extract_epi8(offset, idx * 8);
        prev.gc_epoch = _mm256_extract_epi32(gc_epoch, idx);
        prev.is_write = _mm256_extract_epi8(is_write, idx * 8);

        fprintf(stderr, "data race!");

        return true;
    }



    return false;

}

#endif

void JtsanRTL::MemoryAccess(void *addr, Method *m, address &bcp, uint8_t access_size, bool is_write) {
    JavaThread *thread = JavaThread::current();
    uint16_t tid       = JavaThread::get_jtsan_tid(thread);
    
    uint32_t epoch = JTSanThreadState::getEpoch(tid, tid);
    // create a new shadow cell
    ShadowCell cur = {tid, epoch, (uint8_t)((uptr)addr & (8 - 1)), get_gc_epoch(), is_write, 0};
    // symbolize the access
    // 1 is read, 2 is write
    Symbolizer::Symbolize((Event)(cur.is_write + 1), addr, m->bci_from(bcp), tid);

    // race
    ShadowCell prev;
    JTSanStackTrace *stack_trace = nullptr;
    if (CheckRaces(thread, stack_trace, addr, cur, prev) && !JTSanSilent) {
        JTSanReport::do_report_race(stack_trace, addr, access_size, bcp, m, cur, prev);
    }
}