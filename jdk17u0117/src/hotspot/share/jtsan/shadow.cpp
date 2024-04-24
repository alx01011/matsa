#include "shadow.hpp"

#include <cstdlib>
#include <cstdio>
#include <cstring>

#include "runtime/atomic.hpp"
#include "memory/universe.hpp"
#include "gc/parallel/parallelScavengeHeap.hpp"
#include "gc/shared/collectedHeap.hpp"
#include "runtime/os.hpp"

#define MAX_CAS_ATTEMPTS (100)
#define SHADOW_RATE (4.0/8.0) // 4 shadow cells per dword

#define GB_TO_BYTES(x) ((x) * 1024UL * 1024UL * 1024UL)

ShadowMemory* ShadowMemory::shadow = nullptr;

ShadowMemory::ShadowMemory(size_t size, void *shadow_base, uptr offset, uptr heap_base) {
    this->size              = size;
    this->shadow_base       = shadow_base;
    this->offset            = offset;
    this->heap_base         = heap_base;
}

ShadowMemory::~ShadowMemory() {
    os::unmap_memory((char*)this->shadow_base, this->size);

    ShadowMemory::shadow = nullptr;
}

void ShadowMemory::init(size_t bytes) {
    /*
        Since we already know the size of the java heap, the virtual address space doesn't need to be too big.
        We just need to multiply the size of the heap by the shadow scale and we are good to go.
        Shadow scale is the number of bits we shift to the right to get the shadow memory address.
        For now it is equal to the number of shadow cells per word.
    */  

    if (sizeof(ShadowCell) != sizeof(uint64_t)) {
        fprintf(stderr, "JTSAN: ShadowCell size is not 64bits\n");
        exit(1);
    }

    if (Universe::heap()->kind() != CollectedHeap::Name::Parallel) {
        fprintf(stderr, "JTASN: Only Parallel GC is supported\n");
        exit(1);
    }

    ParallelScavengeHeap *heap = ParallelScavengeHeap::heap();
    uptr base = (uptr)heap->base();

    /*
        total number of locations = HeapSize / 8
        total_shadow = total number of locations * 32 (4 * sizeof(ShadowCell) = 32)
        so total_shadow = HeapSize * 4
    */
    size_t shadow_size  = SHADOW_CELLS * bytes; 

    void *shadow_base  = os::reserve_memory(shadow_size);
    
    /*
        Address returned by os::reserve_memory is allocated using PROT_NONE, which means we can't access it.
        Change the protection to read/write so we can use it.
    */
    bool protect = os::protect_memory((char*)shadow_base, shadow_size, os::MEM_PROT_RW);

    if (shadow_base == nullptr || !protect) {
        fprintf(stderr, "JTSAN: Failed to allocate shadow memory\n");
        exit(1);
    }

    ShadowMemory::shadow = new ShadowMemory(shadow_size, shadow_base, (uptr)shadow_base, base);
}

void ShadowMemory::destroy(void) {
    if (ShadowMemory::shadow != nullptr) {
        delete ShadowMemory::shadow;
    }
}


ShadowMemory* ShadowMemory::getInstance(void) {
    return ShadowMemory::shadow;
}

void* ShadowMemory::MemToShadow(uptr mem, uint8_t size) {
    uptr index = ((uptr)mem - (uptr)this->heap_base) / size; // size is the size of the reference
    uptr shadow_offset = index * 32; // Each metadata entry is 8 bytes 

    return (void*)((uptr)this->offset + shadow_offset);
}

ShadowCell cell_load_atomic(ShadowCell *cell) {
    ShadowCell ret;
    
    uint64_t word = Atomic::load((uint64_t*)cell);
    memcpy(&ret, &word, sizeof(uint64_t));

    return ret;
}

void cell_store_atomic(ShadowCell *cell, ShadowCell *val) {
    uint64_t word = 0;
    word = *(uint64_t*)val;
    //memcpy(&word, val, sizeof(uint64_t));
    // memcpy might not be necessary

    Atomic::store((uint64_t*)cell, word);
}

ShadowCell ShadowBlock::load_cell(uptr mem, uint8_t size, uint8_t index) {
    ShadowMemory *shadow = ShadowMemory::getInstance();
    void *shadow_addr = shadow->MemToShadow(mem, size);

    ShadowCell *cell_ref = &((ShadowCell *)shadow_addr)[index];

    if ((uptr)cell_ref < (uptr)shadow->shadow_base || (uptr)cell_ref >= (uptr)shadow->shadow_base + shadow->size) {
        fprintf(stderr, "Shadow memory out of bounds in load_cell with index %d\n", index);
        exit(1);
    }

    return cell_load_atomic(cell_ref);
}

void ShadowBlock::store_cell(uptr mem, uint8_t size, ShadowCell* cell) {
    ShadowMemory *shadow = ShadowMemory::getInstance();
    void *shadow_addr = shadow->MemToShadow(mem, size);

    ShadowCell *cell_addr = (ShadowCell *)((uptr)shadow_addr);

    // find the first free cell
    for (uint8_t i = 0; i < SHADOW_CELLS; i++) {
        ShadowCell cell_l = cell_load_atomic(&cell_addr[i]);
        /*
          * Technically, an epoch can never be zero, since the epoch is incremented before each access
          * So a zero epoch means the cell is free.
        */
        if (!cell_l.epoch) {
            cell_store_atomic(&cell_addr[i], cell);
            return;
        }
    }

    // if we reach here, all the cells are occupied or locked
    // just pick one at random and overwrite it
    uint8_t ci = os::random() % SHADOW_CELLS;
    cell_addr = &cell_addr[ci];

    cell_store_atomic(cell_addr, cell);
}


