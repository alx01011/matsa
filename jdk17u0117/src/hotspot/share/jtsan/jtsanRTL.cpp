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
        uint32_t thr = JtsanThreadState::getEpoch(cur.tid, cell.tid);

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
      uint8_t index = JtsanThreadState::getHistory(cur.tid)->index.load(std::memory_order_relaxed) % SHADOW_CELLS;
      ShadowBlock::store_cell_at((uptr)addr, &cur, index);
    }

    return isRace;
}
#else // JTSAN_VECTORIZE
bool JtsanRTL::CheckRaces(JavaThread *thread, JTSanStackTrace* &trace, void *addr, ShadowCell &cur, ShadowCell &prev) {
    uptr addr_aligned = ((uptr)addr);

    bool stored   = false;
    bool isRace   = false;

    // we can perform a single load into a 256-bit register
    // and then compare the values

    m256 shadow       = _mm256_load_si256((const __m256i *)addr_aligned);

/*
    // // Print the shadow vector
    // uint64_t shadow_ptr = (uint64_t)&shadow_vec;
    // for (int i = 0; i < 4; i++) {
    //     printf("Shadow[%d]: tid=%d, epoch=%d, offset=%d, gc_epoch=%d, is_write=%d, is_ignored=%d\n",
    //         i, 
    (tid) shadow_ptr[i] & 0xFF, 
    (epoch) (shadow_ptr[i] >> 8 & 0xFFFFFFFF,
     (offset) (shadow_ptr[i] >> 40) & 0x7,
    (gc_epoch) (shadow_ptr[i] >> 43) & 0x7FFFF,
     (is_write) (shadow_ptr[i] >> 62) & 0x1,
     (is_ignored) (shadow_ptr[i] >> 63) & 0x1);
    // }
*/

    uint64_t *cell = (uint64_t *)&shadow;
#define CHECK_CELL(idx)\
    {\
        if (((cell[idx] >> 8 ) & 0xFFFFFFFF) && ((cell[idx] >> 43) & 0x7FFFF) == cur.gc_epoch && ((cell[idx] >> 40) & 0x7) == cur.offset) {\
            if ((cell[idx] & 0xFF) != cur.tid) {\
                uint32_t thr = JtsanThreadState::getEpoch(cur.tid, (cell[idx] >> 8) & 0xFFFFFFFF);\

                if (thr < ((cell[idx] >> 8) & 0xFFFFFFFF)) {\
                    prev = *(ShadowCell *)&cell[idx];\
                    
                    isRace = true;\
                    trace = new JTSanStackTrace(thread);\
                    if (LIKELY(JTSanSuppression::is_suppressed(trace))) {\
                        isRace = false;\
                    }\
                    cur.is_ignored = 1;\
                    stored = true;\
                    ShadowBlock::store_cell_at((uptr)addr, &cur, 0);\
                    goto DONE;\
                }\
            }\
            else {\
                if (cur.is_write && !(cell[idx] >> 62 & 0x1)) {\
                    ShadowBlock::store_cell_at((uptr)addr, &cur, idx);
                    stored = true;\
                }\
            }\
        }\
    }

    CHECK_CELL(0);
    CHECK_CELL(1);
    CHECK_CELL(2);
    CHECK_CELL(3);

DONE:

    if (UNLIKELY(!stored)) {
    // store the shadow cell
      uint8_t index = JtsanThreadState::getHistory(cur.tid)->index.load(std::memory_order_relaxed) % SHADOW_CELLS;
      ShadowBlock::store_cell_at((uptr)addr, &cur, index);
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