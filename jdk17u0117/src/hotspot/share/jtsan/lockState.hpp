#ifndef LOCKSTATE_HPP
#define LOCKSTATE_HPP

#include "threadState.hpp"
#include "vectorclock.hpp"

#include <cstddef>
#include <cstdint>

#include "memory/allocation.hpp"

// struct LockState {
//     uint32_t epoch[MAX_THREADS];

//     // this is used to track the thread that we have modified the vector clock (epoch)
//     // allows for faster max operation on lock
//     uint16_t modified[MAX_THREADS];
//     // look up on unlock will still be slow but modifiedSize can't get too big
//     uint16_t modifiedSize;
// };

class LockShadow : public CHeapObj<mtInternal>{
    private:
        static LockShadow* objectInstance;
        static LockShadow* syncInstance;

        void    *addr; // starting address
        uint32_t nmemb; // number of locks
        size_t   size; // size in bytes
        
        uint32_t   cur; // current lock index

    public:
        // shadow memory for object locks
        static LockShadow* ObjectLockShadow(void);
        // shadow memory for sync locks
        static LockShadow* SyncLockShadow(void);

        static void init(void);
        static void destroy(void);

        Vectorclock* indexToLockVector(uint32_t index);
        
        uint32_t getCurrentLockIndex(void);

        void incrementLockIndex(void);

        // transfers vector clock from tid to lock object
        void transferVectorclock(size_t tid, uint32_t index);

        void transfer_vc(size_t tid);

        Vectorclock* get_vectorclock(void);

        LockShadow(void);
        ~LockShadow(void);
};

#endif