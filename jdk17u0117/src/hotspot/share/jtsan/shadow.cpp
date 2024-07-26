#include "shadow.hpp"
#include "threadState.hpp"
#include "jtsanDefs.hpp"

#include <cstdlib>
#include <cstdio>
#include <cstring>

#include "memory/universe.hpp"
#include "gc/parallel/parallelScavengeHeap.hpp"
#include "gc/shared/collectedHeap.hpp"
#include "runtime/os.hpp"

#include <atomic>

#define MAX_CAS_ATTEMPTS (100)
#define GB_TO_BYTES(x) ((x) * 1024UL * 1024UL * 1024UL)

uptr   ShadowMemory::heap_base   = 0ull;
size_t ShadowMemory::size        = 0ull;
void*  ShadowMemory::shadow_base = nullptr;


void ShadowMemory::init(size_t bytes) {
    /*
        Since we already know the size of the java heap, the virtual address space doesn't need to be too big.
        We just need to multiply the size of the heap by the shadow scale and we are good to go.
        Shadow scale is the number of bits we shift to the right to get the shadow memory address.
        For now it is equal to the number of shadow cells per word.
    */  

    if (sizeof(ShadowCell) != 8) {
        fprintf(stderr, "JTSAN: ShadowCell size is not 64bits\n");
        exit(1);
    }

    if (sizeof(void*) != 8) {
        fprintf(stderr, "JTSAN: Only 64bit systems are supported\n");
        exit(1);
    }

    if (Universe::heap()->kind() == CollectedHeap::Name::Z) {
        fprintf(stderr, "JTSAN: ZGC is not supported\n");
        exit(1);
    }

    // ParallelScavengeHeap *heap = ParallelScavengeHeap::heap();
    // uptr base = (uptr)heap->base();

    uptr base = (uptr)Universe::heap()->reserved_region().start();

    /*
        total number of locations = HeapSize / 8
        total_shadow = total number of locations * 32 (4 * sizeof(ShadowCell) = 32)
        so total_shadow = HeapSize * 4
    */
    size_t shadow_size = bytes * 4;

    void *shadow_base = os::reserve_memory(shadow_size);
    
    /*
        Address returned by os::reserve_memory is allocated using PROT_NONE, which means we can't access it.
        Change the protection to read/write so we can use it.
    */
    bool protect = os::protect_memory((char*)shadow_base, shadow_size, os::MEM_PROT_RW);

    if (shadow_base == nullptr || !protect) {
        fprintf(stderr, "JTSAN: Failed to allocate shadow memory\n");
        exit(1);
    }

    // check for memory alignment
    if ((uptr)shadow_base & 0x7) { // shadow_base % 8
        fprintf(stderr, "JTSAN: Shadow memory is not aligned\n");
        exit(1);
    }

    ShadowMemory::size        = shadow_size;
    ShadowMemory::shadow_base = shadow_base;
    ShadowMemory::heap_base   = base;
}

void ShadowMemory::destroy(void) {
    os::unmap_memory((char*)ShadowMemory::shadow_base, ShadowMemory::size);
}


void* ShadowMemory::MemToShadow(uptr mem) {
    uptr index = ((uptr)mem - (uptr)ShadowMemory::heap_base) / 8; // index in heap
    uptr shadow_offset = index * 32; // Each metadata entry is 8 bytes 

    return (void*)((uptr)ShadowMemory::shadow_base + shadow_offset);
}

ShadowCell ShadowBlock::load_cell(uptr mem, uint8_t index) {
    void *shadow_addr = ShadowMemory::MemToShadow(mem);

    //ShadowCell *cell_ref = &((ShadowCell *)shadow_addr)[index];
    //return *cell_ref;

    uint64_t raw_cell = __atomic_load_n((uint64_t*)((uptr)shadow_addr + (index * sizeof(ShadowCell))), __ATOMIC_RELAXED);
    return *(ShadowCell*)&raw_cell;
}

void *ShadowBlock::store_cell(uptr mem, ShadowCell* cell) {
    void *shadow_addr = ShadowMemory::MemToShadow(mem);

    //ShadowCell *cell_addr = (ShadowCell *)((uptr)shadow_addr);

    // if we reach here, all the cells are occupied
    // just pick one at random and overwrite it
    uint8_t ci = JTSanThreadState::getHistory(cell->tid)->index % SHADOW_CELLS;
    void *store_addr = (void*)((uptr)shadow_addr + (ci * sizeof(ShadowCell)));

    __atomic_store_n((uint64_t*)store_addr, *(uint64_t*)cell, __ATOMIC_RELAXED);
    return store_addr;
}

void ShadowBlock::store_cell_at(uptr mem, ShadowCell* cell, uint8_t index) {
    void *shadow_addr = ShadowMemory::MemToShadow(mem);

    //ShadowCell *cell_addr = &((ShadowCell *)shadow_addr)[index];
    //*cell_addr = *cell;

    void *store_addr = (void*)((uptr)shadow_addr + (index * sizeof(ShadowCell)));
    __atomic_store_n((uint64_t*)store_addr, *(uint64_t*)cell, __ATOMIC_RELAXED);
}
