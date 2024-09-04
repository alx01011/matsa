#ifndef LOCKSTATE_HPP
#define LOCKSTATE_HPP

#include "vectorclock.hpp"

#include <cstddef>
#include <cstdint>

#include "memory/allocation.hpp"

class LockShadow : public CHeapObj<mtInternal>{
    private:
        void    *addr; // starting address
        size_t   size; // size in bytes

    public:
        void transfer_vc(size_t tid);
        Vectorclock* get_vectorclock(void);

        LockShadow(void);
        ~LockShadow(void);
};

#endif