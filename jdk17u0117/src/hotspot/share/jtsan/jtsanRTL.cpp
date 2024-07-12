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

#else /* JTSAN_VECTORIZE */

bool JtsanRTL::CheckRaces(JavaThread *thread, JTSanStackTrace* &trace, void *addr, ShadowCell &cur, ShadowCell &prev) {
    void *shadow_addr = ShadowMemory::MemToShadow((uptr)addr);

    m256 block = _mm256_load_si256((__m256i*)shadow_addr);
    
    m256 thread_epochs = _mm256_setzero_si256();

    ShadowCell cells[SHADOW_CELLS];

#define LOAD_CELL(i)\
    {\
        uint64_t cell;\
        cell     = _mm256_extract_epi64(block, i);\
        cells[i] = *((ShadowCell*)&cell);\
    }

    LOAD_CELL(0);
    LOAD_CELL(1);
    LOAD_CELL(2);
    LOAD_CELL(3);

#undef LOAD_CELL
    
    bool stored = false;
    bool isRace = false;

    for (uint8_t i = 0; i < SHADOW_CELLS; i++) {
        if (LIKELY(cells[i].epoch == 0 || cur.gc_epoch != cells[i].gc_epoch || cells[i].offset != cur.offset)) {
            continue;
        }

        if (UNLIKELY(cells[i].is_ignored)) {
            return false;
        }

        if (LIKELY(cells[i].tid == cur.tid)) {
            if (LIKELY(cur.is_write && !cells[i].is_write)) {
                ShadowBlock::store_cell_at((uptr)addr, &cur, i);
                stored = true;
            }
            continue;
        }

        if (LIKELY(!(cells[i].is_write || cur.is_write))) {
            continue;
        }

        uint32_t thr = JTSanThreadState::getEpoch(cur.tid, cells[i].tid);
        if (LIKELY(thr >= cells[i].epoch)) {
            continue;
        }

        prev   = cells[i];
        isRace = true;

        trace = new JTSanStackTrace(thread);
        if (LIKELY(JTSanSuppression::is_suppressed(trace))) {
            isRace = false;
        }

        cur.is_ignored = 1;
        ShadowBlock::store_cell_at((uptr)addr, &cur, 0);
        stored = true;

        break;
    }

    if (UNLIKELY(!stored)) {
        uint8_t index = JTSanThreadState::getHistory(cur.tid)->index.load(std::memory_order_relaxed) % SHADOW_CELLS;
        ShadowBlock::store_cell_at((uptr)addr, &cur, index);
    }

    return isRace;
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
    Symbolizer::Symbolize((Event)(cur.is_write + 1), addr, m->bci_from(bcp), tid, epoch);

    // race
    ShadowCell prev;
    JTSanStackTrace *stack_trace = nullptr;
    if (CheckRaces(thread, stack_trace, addr, cur, prev) && !JTSanSilent) {
        JTSanReport::do_report_race(stack_trace, addr, access_size, bcp, m, cur, prev);
    }
}