/*
    * MaTSa: Java Thread Sanitizer
    * Author: Alexandros Antonakakis, University of Crete / FORTH
    * This file contains the shadow memory implementation
    * 
    * The shadow memory is a memory region that is used to store metadata
    * about the application memory. This metadata is used to track the
    * state of the application memory and to detect data races.

*/

#ifndef SHADOW_HPP
#define SHADOW_HPP
#include <cstddef>  

#include "memory/allocation.hpp"
#include "runtime/mutex.hpp"

#define SHADOW_CELLS (4)

/* TODO: check for 32bit or 64bit
currently we only tested on linux x86_64
32bit shouldnt really be a problem for small heaps
*/
typedef uintptr_t uptr;

class ShadowMemory : AllStatic {
    public:
        static void *shadow_base; // starting address
        static void *shadow_history_base; // starting address of the stack shadow memory
        static size_t size; // size in bytes

        static uptr heap_base; // base address of the heap

        static void init(size_t bytes);
        static void reset(void);
        static void destroy(void);

        static void *MemToShadow(uptr mem);
        static void *MemToHistory(uptr mem);
};

/*
    * The shadow memory is divided into shadow cells.
    * Each shadow cell is a 64bit value that contains the thread id
    * and the epoch of the last access to the memory location.
    * 
    * The shadow memory is a 4x larger than the application memory.
    * This means that for each word (8bytes) of application memory we have
    * 4 shadow cells 64bits each, so 32bytes of shadow memory, per word.
    * 
    * The shadow memory is used to track the state of the accessed memory.
*/

// 64bit shadow cell
struct ShadowCell {
    uint64_t tid        : 17;
    uint64_t epoch      : 42;
    uint64_t offset     : 3; // 0-7 in case of 1,2 or 4 byte access
    uint64_t is_write   : 1;
    uint64_t is_ignored : 1; // in case of a suppressed race
};

struct HistoryCell {
    uint64_t bci           : 16;
    uint64_t ring_idx      : 16;
    uint64_t history_idx   : 16;
    uint64_t history_epoch : 16;
};

class ShadowBlock : AllStatic {
    public:
        static ShadowCell  load_cell(uptr mem, uint8_t index);
        static HistoryCell load_history(uptr mem, uint8_t index);
        static void        store_cell(uptr mem, ShadowCell* cell, HistoryCell* history); 
        static void        store_cell_at(uptr mem, ShadowCell* cell, HistoryCell* history, uint8_t index);
};

#endif