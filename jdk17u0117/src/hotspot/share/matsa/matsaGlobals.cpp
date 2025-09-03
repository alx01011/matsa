#include "matsaGlobals.hpp"
#include "runtime/atomic.hpp"

#include <cstdio>

uint64_t env_event_buffer_size = 1 << 17;
uint64_t matsa_history_size = 6;

void matsa_at_exit_handler(void) {
    fprintf(stderr, "MaTSa: reported %lu warnings\n", COUNTER_GET(race));
}

#define COUNTER_DEF(x)\
    uint64_t MaTSaStats::x##_counter = 0;

COUNTER_DEF(race);