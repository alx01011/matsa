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

#define SHADOW_CELLS (4)

/* TODO: check for 32bit or 64bit
currently we only tested on linux x86_64
32bit shouldnt really be a problem for small heaps
*/
typedef uintptr_t uptr;

struct MemAddr {
    uptr addr;
    uptr size;
};

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

        /*
            Since address might not always be associated with 8byte words
            we allocate 4 different spaces:

            1 byte shadow space
            2 byte shadow space
            4 byte shadow space
            8 byte shadow space
        */

        static ShadowMemory *instances[9];

        void *MemToShadow(MemAddr mem);
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
    uint16_t tid      : 16;
    uint64_t epoch    : 42;
    uint8_t  gc_epoch : 5;
    uint8_t  is_write : 1;
};

class ShadowBlock : AllStatic {
    public:
        static ShadowCell  load_cell(MemAddr mem, uint8_t index);
        static void       store_cell(MemAddr mem, ShadowCell* cell); 
};

#endif