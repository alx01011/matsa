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
#include "utilities/globalDefinitions.hpp"

#include "runtime/interfaceSupport.inline.hpp"

#include <immintrin.h>

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
        uint64_t thr = MaTSaThreadState::getEpoch(cur.tid, cell.tid);

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

#if MATSA_VECTORIZE
bool MaTSaRTL::FastCheckRaces(void *addr, int32_t bci, ShadowCell &cur, ShadowCell &prev, HistoryCell &prev_history) {
    bool stored   = false;
    bool isRace   = false;

    HistoryCell cur_history = {(uint64_t)bci, History::get_part_idx(cur.tid),
                                 History::get_event_idx(cur.tid), History::get_cur_epoch(cur.tid)};

 
    m256 s_cells = _mm256_loadu_si256((m256*)ShadowMemory::MemToShadow((uptr)addr)); // Load 4 shadow cells into a 256-bit register

    // bit masks to extract fields (matching struct layout)
    const m256 mask_tid        = _mm256_set1_epi64x((1ULL << 17) - 1);                // bits 0-16
    const m256 mask_epoch      = _mm256_set1_epi64x(((1ULL << 42) - 1) << 17);        // bits 17-58 (0b000001111111111111111111111111111111111111111110000000000000000)
    const m256 mask_offset     = _mm256_set1_epi64x(7ULL << 59);                      // bits 59-61 (0b111 shifted left by 59 places leaves bits 59-61 set)
    const m256 mask_is_write   = _mm256_set1_epi64x(1ULL << 62);                      // bit 62
    const m256 mask_is_ignored = _mm256_set1_epi64x(1ULL << 63);                      // bit 63

    // vectors of each field
    const m256 tids       = _mm256_and_si256(s_cells, mask_tid);
    const m256 epochs     = _mm256_and_si256(s_cells, mask_epoch);
    const m256 offsets    = _mm256_and_si256(s_cells, mask_offset);
    const m256 is_writes  = _mm256_and_si256(s_cells, mask_is_write);
    const m256 is_ignored = _mm256_and_si256(s_cells, mask_is_ignored);

    // a vector for current cell
    const m256 cur_v = _mm256_set1_epi64x(*(uint64_t*)&cur);

    const m256 cur_tid       = _mm256_and_si256(cur_v, mask_tid);
    const m256 cur_offset    = _mm256_and_si256(cur_v, mask_offset);
    const m256 cur_is_write  = _mm256_and_si256(cur_v, mask_is_write);

    
    const m256 zeros = _mm256_setzero_si256();
    const m256 ones  = _mm256_set1_epi64x(~0ULL); // all bits set to 1

    // before running any checks, see if there is a cell with the same offset and ignored bit set
    m256 same_offset   = _mm256_cmpeq_epi64(offsets, cur_offset);
    m256 ignored_cells = _mm256_and_si256(same_offset, is_ignored);

    if (_mm256_movemask_epi8(ignored_cells)) {
        // if there is a cell with the same offset and ignored bit set, we can skip the rest
        return false;
    }

    // first get rid of empty cells (epoch == 0)
    m256 empty_cells = _mm256_cmpeq_epi64(epochs, zeros);
    int empty_mask = _mm256_movemask_epi8(empty_cells);

    // we can store in an empty cell
    if (LIKELY(empty_mask)) {
        int i = __builtin_ffs(empty_mask) / 8; // find the first empty cell
        ShadowBlock::store_cell_at((uptr)addr, &cur, &cur_history, i);
        stored = true;
    }

    // different offsets
    // xor with ones (all bits set) to flip all bits (aka NOT operation)
    m256 different_offsets = _mm256_xor_si256(_mm256_cmpeq_epi64(offsets, cur_offset), ones);
    // ignore same tid
    m256 same_tid = _mm256_cmpeq_epi64(tids, cur_tid);
    // ignore both reads
    m256 cur_read   = _mm256_cmpeq_epi64(cur_is_write, zeros);
    m256 cell_reads = _mm256_cmpeq_epi64(is_writes, zeros);
    m256 both_reads = _mm256_and_si256(cur_read, cell_reads);

    // if the tid and offset is the same we can replace weaker accesses (if current is write and previous is read)
    // m256 weaker_access = _mm256_andnot_si256(cur_read, 
    //             _mm256_and_si256(same_offset, _mm256_and_si256(cell_reads, same_tid)));
    m256 weaker_access = _mm256_and_si256(same_offset, _mm256_and_si256(cell_reads, same_tid));
    
    int weaker_mask = _mm256_movemask_epi8(weaker_access);
    if (weaker_mask && cur.is_write) {
        int i = __builtin_ffs(weaker_mask) / 8; // find the first weaker access
        ShadowBlock::store_cell_at((uptr)addr, &cur, &cur_history, i);
        stored = true;
    }

    m256 skip = _mm256_or_si256(different_offsets, _mm256_or_si256(same_tid, both_reads));

    // candidates : !empty && !skip
    // _mm256_andnot_si256(empty_cells, ones) -> this is the NOT of empty_cells, (~empty_cells & 1 == ~empty_cells)
    m256 candidates = _mm256_andnot_si256(skip, _mm256_andnot_si256(empty_cells, ones));

    /*
        * this pretty much splits the candidates into 8 byte chunks
        * if a cell is ignored, its chunk will be 0 (all 8 bytes 0)
        * if its a candidates, it will be 0xff... (all 8 bytes 1)
    */
    int candidate_mask = _mm256_movemask_epi8(candidates);

    if (candidate_mask) {
        m256 thread_epochs = _mm256_set1_epi64x(0);
// idea taken from llvm's tsan
#define LOAD_EPOCH(i) \
        if ((candidate_mask >> (i * 8)) & 0xFF) {\
            uint64_t tid   = _mm256_extract_epi64(tids, i); \
            uint64_t epoch = ((uint64_t)MaTSaThreadState::getEpoch(cur.tid, tid)) << 17; \
            thread_epochs = _mm256_insert_epi64(thread_epochs, epoch, i); \
        }
        LOAD_EPOCH(0);
        LOAD_EPOCH(1);
        LOAD_EPOCH(2);
        LOAD_EPOCH(3);
#undef LOAD_EPOCH
        /*
            * thread_epochs < epochs
            * Note: This is a signed comparison
            * because epochs are 42 bits the sign bit is always 0
            * so we can safely use _mm256_cmpgt_epi64.
            * 
            * No need to shift epochs (cells) as thread_epochs is shifted to mimic
            * how the epoch is stored in the shadow_t struct.
        */
        m256 racy = _mm256_cmpgt_epi64(epochs, thread_epochs); 
        racy = _mm256_and_si256(racy, candidates); // only keep candidates that are racy
        
        int racy_mask = _mm256_movemask_epi8(racy);
        if (racy_mask) {
            int i = __builtin_ffs(racy_mask) / 8; // find the first candidate

            prev = ShadowBlock::load_cell((uptr)addr, i);
            prev_history = ShadowBlock::load_history((uptr)addr, i);

            // mark ignore flag and store at ith index
            // so we can skip if ever encountered again
            // its fine if we miss it, we also check for previously reported races in do_report
            cur.is_ignored = 1;
            ShadowBlock::store_cell_at((uptr)addr, &cur, &cur_history, i);
            stored = true;
            isRace = true;
        }
    }

    if (UNLIKELY(!stored)) {
        // store the shadow cell
        ShadowBlock::store_cell((uptr)addr, &cur, &cur_history);
    }

    return isRace;
}
#else 
bool MaTSaRTL::FastCheckRaces(void *addr, int32_t bci, ShadowCell &cur, ShadowCell &prev, HistoryCell &prev_history) {
    // do nothing
    return false;
}
#endif // MATSA_VECTORIZE


void MaTSaRTL::InterpreterMemoryAccess(void *addr, Method *m, address bcp, uint8_t access_size, bool is_write) {
    JavaThread *thread  = JavaThread::current();
    uint64_t tid        = JavaThread::get_matsa_tid(thread);
    
    int32_t  bci   = m->bci_from(bcp);
    uint32_t epoch = MaTSaThreadState::getEpoch(tid, tid);
    // create a new shadow cell
    ShadowCell cur = {tid, epoch, (uint8_t)((uptr)addr & (8 - 1)), is_write, 0};
    
    // race
    ShadowCell prev;
    HistoryCell prev_history;
#if MATSA_VECTORIZE
    bool is_race = MaTSaRTL::FastCheckRaces(addr, bci, cur, prev, prev_history);
#else
    bool is_race = CheckRaces(addr, bci, cur, prev, prev_history);
#endif
    if (is_race && !MaTSaSilent) {
        MaTSaReport::do_report_race(thread, addr, access_size, bci, m, cur, prev, prev_history);
    }
}

JRT_LEAF(void, MaTSaRTL::C1MemoryAccess(void *addr, Method *m, int bci, uint8_t access_size, bool is_write))
    JavaThread *thread = JavaThread::current();
    uint64_t tid       = JavaThread::get_matsa_tid(thread);

    uint64_t epoch = MaTSaThreadState::getEpoch(tid, tid);
    // create a new shadow cell
    ShadowCell cur = {tid, epoch, (uint8_t)((uptr)addr & (8 - 1)), is_write, 0};

    // race
    ShadowCell prev;
    HistoryCell prev_history;
#if MATSA_VECTORIZE
    bool is_race = MaTSaRTL::FastCheckRaces(addr, bci, cur, prev, prev_history);
#else
    bool is_race = CheckRaces(addr, bci, cur, prev, prev_history);
#endif

    if (is_race && !MaTSaSilent) {
        MaTSaReport::do_report_race(thread, addr, access_size, bci, m, cur, prev, prev_history);
    }
JRT_END

JRT_LEAF(void, MaTSaRTL::C2MemoryAccess(void *addr, Method *m, int bci, uint8_t access_size, bool is_write))
    JavaThread *thread = JavaThread::current();
    uint64_t tid       = JavaThread::get_matsa_tid(thread);

    uint64_t epoch = MaTSaThreadState::getEpoch(tid, tid);
    // create a new shadow cell
    ShadowCell cur = {tid, epoch, (uint8_t)((uptr)addr & (8 - 1)), is_write, 0};

    // race
    ShadowCell prev;
    HistoryCell prev_history;
#if MATSA_VECTORIZE
    bool is_race = MaTSaRTL::FastCheckRaces(addr, bci, cur, prev, prev_history);
#else
    bool is_race = CheckRaces(addr, bci, cur, prev, prev_history);
#endif

    if (is_race && !MaTSaSilent) {
        MaTSaReport::do_report_race(thread, addr, access_size, bci, m, cur, prev, prev_history);
    }
JRT_END