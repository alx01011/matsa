#ifndef JTSAN_REPORT_MAP_HPP
#define JTSAN_REPORT_MAP_HPP

#include <cstddef>

#include "runtime/mutex.hpp"
#include "memory/allocation.hpp"

// saving the last 64 reports
#define MAX_MAP_SIZE (1 << 6)

// FIXME
// this is pretty naive, we could do a circular buffer along with a map for faster access
class JtsanReportMap : public CHeapObj<mtInternal> {
    private:
        class hashmap : public CHeapObj<mtInternal> {
            // value is the bcp of the access
            public:
                void* value;
                hashmap* next;
        };

        hashmap* _map[MAX_MAP_SIZE];
        size_t   _size;

        Mutex* _lock;

        static JtsanReportMap* instance;
    public:
        JtsanReportMap();
        ~JtsanReportMap();
        void  put    (void* addr);
        void* get    (void* addr);
        void  clear  (void);

        static JtsanReportMap* get_instance(void);
        static void jtsan_reportmap_destroy(void);
        static void jtsan_reportmap_init(void);
};

#endif