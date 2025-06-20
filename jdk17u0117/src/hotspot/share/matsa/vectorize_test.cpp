// #include <immintrin.h> // Required for AVX2 intrinsics
// #include <cstdint>
// #include <iostream>

// using uptr = uintptr_t;

// // 64bit shadow cell from the user
// struct ShadowCell {
//     uint64_t tid        : 17;
//     uint64_t epoch      : 42;
//     uint64_t offset     : 3; // 0-7 in case of 1,2 or 4 byte access
//     uint64_t is_write   : 1;
//     uint64_t is_ignored : 1; // in case of a suppressed race
// };

// // A simple stand-in for history data
// struct History {
//     uint64_t data[4];
// };

// // --- Mock Implementations for Compilation ---
// // These are mock-ups of the external functions to make the example runnable.
// // You would replace these with your actual implementations.

// namespace ShadowBlock {
//     // Mock shadow memory. In a real scenario, this would be allocated elsewhere.
//     alignas(32) ShadowCell shadow_memory[4];
//     History history_memory[4];

//     // Loads all 4 cells at once. Assumes addr points to a 32-byte aligned block.
//     __m256i load_block(uptr addr) {
//         return _mm256_load_si256((const __m256i*)addr);
//     }

//     void store_cell_at(uptr addr, const ShadowCell* cell, const History* history, int index) {
//         std::cout << "Storing at index " << index << "...\n";
//         ((ShadowCell*)addr)[index] = *cell;
//         history_memory[index] = *history; // Not part of the core logic, but included for completeness
//     }

//     History load_history(uptr addr, int index) {
//         return history_memory[index];
//     }
// } // namespace ShadowBlock

// namespace MaTSaThreadState {
//     // Mock epoch retrieval.
//     uint32_t getEpoch(uint64_t cur_tid, uint64_t cell_tid) {
//         // In a real scenario, this would involve a lookup.
//         // For this example, let's return a value that allows a race to be found.
//         return (cur_tid == 1 && cell_tid == 2) ? 10 : 100;
//     }
// } // namespace MaTSaThreadState

// // --- Vectorized Race Detection Function ---

// bool check_race_vectorized(uptr addr, ShadowCell& cur, History& cur_history, ShadowCell& prev, History& prev_history, bool& isRace) {
//     // Bitmasks to extract fields from a 64-bit integer, matching the ShadowCell layout.
//     const __m256i MASK_TID        = _mm256_set1_epi64x((1ULL << 17) - 1);
//     const __m256i MASK_EPOCH      = _mm256_set1_epi64x(((1ULL << 42) - 1) << 17);
//     const __m256i MASK_OFFSET     = _mm256_set1_epi64x(0b111ULL << 59);
//     const __m256i MASK_IS_WRITE   = _mm256_set1_epi64x(1ULL << 62);
//     const __m256i MASK_IS_IGNORED = _mm256_set1_epi64x(1ULL << 63);

//     // Load the entire 32-byte block (4 x 64-bit cells) into a 256-bit register.
//     // This assumes `addr` is 32-byte aligned for best performance.
//     const __m256i v_cells = _mm256_load_si256((const __m256i*)addr);

//     // --- Part 1: Early exit for 'is_ignored' ---
//     // Check if any cell has the 'is_ignored' flag set.
//     // If so, we can exit immediately, just like the original `return false`.
//     const __m256i v_ignored_flags = _mm256_and_si256(v_cells, MASK_IS_IGNORED);
//     if (!_mm256_testz_si256(v_ignored_flags, v_ignored_flags)) {
//         // At least one 'is_ignored' flag is set.
//         return false;
//     }

//     // --- Part 2: Prepare vectors for comparison ---

//     // Broadcast values from the current access (`cur`) into all 4 lanes of a vector.
//     // We first combine the bitfields of `cur` into a single 64-bit integer.
//     uint64_t cur_as_u64 = *reinterpret_cast<uint64_t*>(&cur);
//     const __m256i v_cur = _mm256_set1_epi64x(cur_as_u64);

//     // Extract fields from `v_cur` to create broadcast vectors for each field.
//     const __m256i v_cur_tid      = _mm256_and_si256(v_cur, MASK_TID);
//     const __m256i v_cur_offset   = _mm256_and_si256(v_cur, MASK_OFFSET);
//     const __m256i v_cur_is_write = _mm256_and_si256(v_cur, MASK_IS_WRITE);

//     // Extract fields from the 4 cells in memory (`v_cells`).
//     const __m256i v_cell_tid      = _mm256_and_si256(v_cells, MASK_TID);
//     const __m256i v_cell_epoch    = _mm256_and_si256(v_cells, MASK_EPOCH);
//     const __m256i v_cell_offset   = _mm256_and_si256(v_cells, MASK_OFFSET);
//     const __m256i v_cell_is_write = _mm256_and_si256(v_cells, MASK_IS_WRITE);

//     // --- Part 3: Generate Masks from Parallel Comparisons ---

//     // Mask 1: Empty cells (cell.epoch == 0)
//     // The epoch bits start at bit 17. An epoch of 0 means the entire epoch field is 0.
//     const __m256i v_zeros = _mm256_setzero_si256();
//     __m256i m_empty = _mm256_cmpeq_epi64(v_cell_epoch, v_zeros);

//     // Mask 2: Mismatched offsets (cell.offset != cur.offset) -> SKIP
//     __m256i m_skip_offset = _mm256_xor_si256(_mm256_cmpeq_epi64(v_cell_offset, v_cur_offset), _mm256_set1_epi64x(-1));

//     // Mask 3: Same thread ID (cell.tid == cur.tid) -> SKIP (mostly)
//     __m256i m_same_tid = _mm256_cmpeq_epi64(v_cell_tid, v_cur_tid);

//     // Mask 4: Both accesses are reads -> SKIP
//     // Skip if !cur.is_write AND !cell.is_write
//     const __m256i m_not_cur_write = _mm256_cmpeq_epi64(v_cur_is_write, v_zeros);
//     const __m256i m_not_cell_write = _mm256_cmpeq_epi64(v_cell_is_write, v_zeros);
//     __m256i m_both_reads = _mm256_and_si256(m_not_cur_write, m_not_cell_write);

//     // --- Part 4: Combine Masks and Find Candidates ---

//     // Combine all skip conditions. A cell is skipped if any of these are true.
//     __m256i m_skip = _mm256_or_si256(m_skip_offset, _mm256_or_si256(m_same_tid, m_both_reads));

//     // A cell is a race candidate if it's NOT empty AND NOT skipped.
//     __m256i m_race_candidates = _mm256_andnot_si256(m_skip, _mm256_andnot_si256(m_empty, _mm256_set1_epi64x(-1)));

//     // Convert the candidate mask to a 32-bit integer mask.
//     // Each 64-bit lane becomes 8 bytes in the mask. A non-zero lane sets the corresponding bytes.
//     // We only need to check one byte per lane, so we can check bits 0, 8, 16, 24.
//     int race_mask = _mm256_movemask_epi8(m_race_candidates);

//     bool stored = false;

//     // --- Part 5: Process Candidates and Empty Cells ---

//     if (race_mask != 0) {
//         // We have at least one candidate. Fall back to scalar code for the epoch check,
//         // as getEpoch() depends on two different TIDs.
//         // Find the index of the first candidate.
//         int i = __builtin_ffs(race_mask) / 8; // ffs is 1-based, and we have 8 bytes per lane

//         ShadowCell cell = ((ShadowCell*)addr)[i];
//         uint32_t thr = MaTSaThreadState::getEpoch(cur.tid, cell.tid);

//         if (thr < cell.epoch) {
//             // This is a confirmed race.
//             prev = cell;
//             prev_history = ShadowBlock::load_history(addr, i);
//             isRace = true;

//             // Mark for ignore and store, as in the original logic.
//             cur.is_ignored = 1;
//             ShadowBlock::store_cell_at(addr, &cur, &cur_history, i);
//             stored = true;
//         }
//     }

//     // Handle storing in an empty slot if no race was found and stored.
//     // Also handles the "stronger write" case for same-TID cells.
//     if (!stored) {
//         // Case 1: Store in the first available empty slot.
//         int empty_mask = _mm256_movemask_epi8(m_empty);
//         if (empty_mask != 0) {
//             int i = __builtin_ffs(empty_mask) / 8;
//             ShadowBlock::store_cell_at(addr, &cur, &cur_history, i);
//             stored = true;
//         } else {
//             // Case 2 (Optional): Handle the "stronger write" case.
//             // This happens if cur.is_write=1, cell.is_write=0, and tid is the same.
//             __m256i m_stronger_write = _mm256_and_si256(m_same_tid, _mm256_and_si256(v_cur_is_write, m_not_cell_write));
//             int stronger_mask = _mm256_movemask_epi8(m_stronger_write);
//             if (stronger_mask != 0) {
//                 int i = __builtin_ffs(stronger_mask) / 8;
//                 ShadowBlock::store_cell_at(addr, &cur, &cur_history, i);
//                 stored = true;
//             }
//         }
//     }
    
//     return true; // Corresponds to the loop finishing without returning false
// }


// // --- Example Usage ---
// int main() {
//     // Initialize mock shadow memory
//     ShadowCell* cells = ShadowBlock::shadow_memory;
    
//     // Scenario 1: Empty block
//     cells[0] = {0, 0, 0, 0, 0};
//     cells[1] = {0, 0, 0, 0, 0};
//     cells[2] = {0, 0, 0, 0, 0};
//     cells[3] = {0, 0, 0, 0, 0};

//     ShadowCell cur1 = {1, 50, 2, 1, 0}; // tid=1, epoch=50, offset=2, write=true
//     History hist1;
//     ShadowCell prev1;
//     History prev_hist1;
//     bool isRace1 = false;
//     std::cout << "--- Scenario 1: Storing in empty cell ---" << std::endl;
//     check_race_vectorized((uptr)cells, cur1, hist1, prev1, prev_hist1, isRace1);
//     std::cout << "Race found: " << std::boolalpha << isRace1 << std::endl;
//     std::cout << "Cell 0 TID after: " << cells[0].tid << std::endl;
//     std::cout << "\n";


//     // Scenario 2: Race condition
//     cells[0] = {2, 20, 2, 1, 0}; // tid=2, epoch=20, offset=2, write=true
//     cells[1] = {0, 0, 0, 0, 0};
//     cells[2] = {3, 90, 1, 0, 0};
//     cells[3] = {0, 0, 0, 0, 0};

//     ShadowCell cur2 = {1, 50, 2, 1, 0}; // tid=1, epoch=50, offset=2, write=true
//     History hist2;
//     ShadowCell prev2;
//     History prev_hist2;
//     bool isRace2 = false;
//     std::cout << "--- Scenario 2: Detecting a race ---" << std::endl;
//     check_race_vectorized((uptr)cells, cur2, hist2, prev2, prev_hist2, isRace2);
//     std::cout << "Race found: " << std::boolalpha << isRace2 << std::endl;
//     if (isRace2) {
//         std::cout << "Previous access TID: " << prev2.tid << ", Epoch: " << prev2.epoch << std::endl;
//         std::cout << "Cell 0 is_ignored flag after: " << cells[0].is_ignored << std::endl;
//     }
//     std::cout << "\n";

//     // Scenario 3: Ignored cell causes early exit
//     cells[0] = {2, 20, 2, 1, 0};
//     cells[1] = {0, 0, 0, 0, 0};
//     cells[2] = {5, 99, 5, 1, 0}; // This cell has the 'is_ignored' flag
//     cells[3] = {0, 0, 0, 0, 0};
//     bool isRace3 = false;
//     std::cout << "--- Scenario 3: Early exit on ignored flag ---" << std::endl;
//     bool result = check_race_vectorized((uptr)cells, cur2, hist2, prev2, prev_hist2, isRace3);
//     std::cout << "Function returned: " << std::boolalpha << result << " (expected false)" << std::endl;

//     return 0;
// }
