/* -*- indent-tabs-mode : nil; c-basic-offset : 4 -*- */
#ifndef CYCLE_H
#define CYCLE_H

static inline
double get_sec(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec/1000000000.0;
}



#ifdef __arm__
#include <unistd.h>
#include <asm/unistd.h>
#include <linux/perf_event.h>

static int fddev = -1;
static inline void
time_init(void)
{
    static struct perf_event_attr attr;
    attr.type = PERF_TYPE_HARDWARE;
    attr.config = PERF_COUNT_HW_CPU_CYCLES;
    fddev = syscall(__NR_perf_event_open, &attr, 0, -1, -1, 0);
}

static inline void
time_end()
{
    close(fddev);
}

static long long
get_cycle()
{
    long long result = 0;
    if (read(fddev, &result, sizeof(result)) != sizeof(long long)) {
        perror("read ev");
        exit(1);
    }
    return result;
}

#else

#include <x86intrin.h>

static inline void time_init(void) { }
static inline void time_end(void) { }

static inline long long
get_cycle()
{
    return __rdtsc();
}

#endif

#endif

