#define _GNU_SOURCE
#include "olta.h"

#include <sched.h>

static cpu_set_t default_affinity;

void affinity_init(void) {
    int r = pthread_getaffinity_np(pthread_self(), sizeof(default_affinity), &default_affinity);
    if (r != 0) {
        log_error("unable to get default affinity");
        assert(0);
    }
}

void affinity_set(int cpu) {
    if (cpu >= 0) {
        cpu_set_t mask;
        CPU_ZERO(&mask);
        CPU_SET(cpu, &mask);
        
        int r = pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask);
        if (r != 0) {
            log_error("unable to set affinity %d", cpu);
        }
    } else {
        int r = pthread_setaffinity_np(pthread_self(), sizeof(default_affinity), &default_affinity);
        if (r != 0) {
            log_error("unable to default affinity");
        }
    } 
}

