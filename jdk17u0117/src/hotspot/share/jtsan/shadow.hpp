/*
    * JTSan: Java Thread Sanitizer
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

class ShadowMemory : public CHeapObj<mtInternal>{
    private:
        static ShadowMemory *shadow; // singleton

        ShadowMemory(size_t size, void *shadow_base, uptr offset, uptr heap_start); // private constructor
        ~ShadowMemory();

    public:
        void *shadow_base; // starting address
        uptr offset;
        size_t size; // size in bytes

        uptr heap_base; // base address of the heap

        static void init(size_t bytes);
        static void destroy(void);

        static ShadowMemory* getInstance(void);

        void *MemToShadow(uptr mem);
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
    uint64_t tid        : 8;
    uint64_t epoch      : 32;
    uint64_t offset     : 3; // 0-7 in case of 1,2 or byte access
    uint64_t gc_epoch   : 19;
    uint64_t is_write   : 1;
    uint64_t is_ignored : 1; // in case of a suppressed race
};

class ShadowBlock : AllStatic {
    public:
        static ShadowCell  load_cell(uptr mem, uint8_t index);
        static void       store_cell(uptr mem, ShadowCell* cell); 
        static void       store_cell_at(uptr mem, ShadowCell* cell, uint8_t index);
    private:
        static ShadowCell atomic_load_cell(ShadowCell *cell);
        static void       atomic_store_cell(ShadowCell *cell, ShadowCell *val);
};

#endif