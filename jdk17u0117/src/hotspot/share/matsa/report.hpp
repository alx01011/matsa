#ifndef REPORT_HPP
#define REPORT_HPP

#include "shadow.hpp"

#include "runtime/mutex.hpp"
#include "oops/method.hpp"
#include "memory/allocation.hpp"
#include "utilities/globalDefinitions.hpp"

#define BUCKETS (1 << 10)

// a hash map to keep track of previously reported races
// unsafe must hold lock before calling any of the methods
class MaTSaReportMap : public CHeapObj<mtInternal> {
    private:
        struct ReportEntry {
            uint64_t bci : 16;
            ReportEntry *next;
        }* _table[BUCKETS];
        uint64_t _size;

        uint64_t hash(uint64_t method);

        static MaTSaReportMap *_instance;

        MaTSaReportMap();
        ~MaTSaReportMap();
    
    public:
        static MaTSaReportMap *instance();

        static void init();
        static void destroy();

        bool contains(uint64_t method, uint64_t bci);
        void insert(uint64_t method, uint64_t bci);
        void clear();
};

namespace MaTSaReport {
    extern Mutex *_report_lock;
    //extern uint8_t _report_lock;

    void do_report_race   (JavaThread *thread, void *addr, uint8_t size, int cur_bci, Method *m, 
                            ShadowCell &cur, ShadowCell &prev, HistoryCell &prev_history);
    void print_current_stack(JavaThread *thread, int cur_bci);
}

#endif