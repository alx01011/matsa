#include "shadow.hpp"

#include <cstdlib>
#include <cstdio>
#include <cstring>

#include "memory/universe.hpp"
#include "gc/parallel/parallelScavengeHeap.hpp"
#include "gc/shared/collectedHeap.hpp"
#include "runtime/os.hpp"

#define MAX_CAS_ATTEMPTS (100)
#define GB_TO_BYTES(x) ((x) * 1024UL * 1024UL * 1024UL)

static const uptr beg = 0x020000000000ull;
static const uptr end = 0x100000000000ull;

const uptr kAppMemMsk     = 0x7c0000000000ull;
const uptr kAppMemXor     = 0x020000000000ull;


ShadowMemory* ShadowMemory::shadow = nullptr;

ShadowMemory::ShadowMemory(size_t size, void *shadow_base, uptr offset, uptr heap_base) {
    this->size              = size;
    this->shadow_base       = shadow_base;
    this->offset            = offset;
    this->heap_base         = heap_base;

    this->_report_lock = new Mutex(Mutex::tty, "JTSAN Report Lock");
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
    //size_t shadow_size  = end - beg;
    size_t shadow_size = bytes * 4;

    //void *shadow_base  = os::attempt_reserve_memory_at((char*)beg, shadow_size);
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

void* ShadowMemory::MemToShadow(uptr mem) {
    uptr index = ((uptr)mem - (uptr)this->heap_base) / 8; // index in heap
    uptr shadow_offset = index * 32; // Each metadata entry is 8 bytes 

    return (void*)((uptr)this->shadow_base + shadow_offset);
}


ShadowCell ShadowBlock::load_cell(uptr mem, uint8_t index) {
    ShadowMemory *shadow = ShadowMemory::getInstance();
    void *shadow_addr = shadow->MemToShadow(mem);

    ShadowCell *cell_ref = &((ShadowCell *)shadow_addr)[index];

    if ((uptr)cell_ref < (uptr)shadow->shadow_base || (uptr)cell_ref >= (uptr)shadow->shadow_base + shadow->size) {
        fprintf(stderr, "Shadow memory (%p) out of bounds in load_cell with index %d\n", shadow_addr, index);
        exit(1);
    }

    return *cell_ref;
}

void ShadowBlock::store_cell(uptr mem, ShadowCell* cell) {
    ShadowMemory *shadow = ShadowMemory::getInstance();
    void *shadow_addr = shadow->MemToShadow(mem);

    ShadowCell *cell_addr = (ShadowCell *)((uptr)shadow_addr);

    // find the first free cell
    for (uint8_t i = 0; i < SHADOW_CELLS; i++) {
        ShadowCell cell_l = cell_addr[i];
        /*
          * Technically, a gc epoch can never be zero, since the gc epoch starts from 1
          * So a zero epoch means the cell is free.
        */
        if (!cell_l.gc_epoch) {
            *(cell_addr + i) = *cell;
            return;
        }
    }

    // if we reach here, all the cells are occupied or locked
    // just pick one at random and overwrite it
    uint8_t ci = os::random() % SHADOW_CELLS;
    cell_addr = &cell_addr[ci];

    *cell_addr = *cell;
}

void ShadowBlock::store_cell_at(uptr mem, ShadowCell* cell, uint8_t index) {
    ShadowMemory *shadow = ShadowMemory::getInstance();
    void *shadow_addr = shadow->MemToShadow(mem);

    ShadowCell *cell_addr = &((ShadowCell *)shadow_addr)[index];

    *cell_addr = *cell;
}

bool ShadowMemory::try_lock_report(void) {
    ShadowMemory *shadow = ShadowMemory::getInstance();

    return shadow->_report_lock->try_lock();
}

void ShadowMemory::unlock_report(void) {
    ShadowMemory *shadow = ShadowMemory::getInstance();

    shadow->_report_lock->unlock();
}


