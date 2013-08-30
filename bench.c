/* -*- indent-tabs-mode : nil; c-basic-offset : 4 -*- */

#define _GNU_SOURCE

#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include "cycle.h"
#include <malloc.h>

#define ITER2(A) A A
#define ITER4(A) A A A A
#define ITER8(A) A A A A A A A A
#define ITER16(A) A A A A A A A A A A A A A A A A

#define ALIGN_UP(v,a) (((v)+((a)-1)) & (~((a)-1)))

struct prog_arg {
    int block_size_op;
    int block_size;
    int nloop;
    int nthread;
    unsigned char *mem1;
    unsigned char *mem2;
    unsigned char *mem3;
};

struct thread_data {
    int tid;
    pthread_t t;

    struct bench *bench;
    struct prog_arg *prog_arg;
};

typedef void (*test_func_t) (struct prog_arg *a, struct thread_data *);
typedef void (*dump_func_t) (struct prog_arg *a, double time_delta, long long cycle_delta);

static void
dump_throughput(struct prog_arg *a, double time_delta, int coef)
{
    double total_size = a->nthread * a->nloop * (double)a->block_size_op * (double)coef;
    double total_size_per_sec = total_size / time_delta;
    double giga = (1.0*1024*1024*1024);
    double mega = (1.0*1024*1024);

    if (total_size_per_sec > giga) {
        printf("%.4f GB/s\n",
               total_size_per_sec / giga);
    } else {
        printf("%.4f MB/s\n",
               total_size_per_sec / mega);
    }
}

static void
dump_2mem(struct prog_arg *a, double time_delta, long long cycle_delta)
{
    dump_throughput(a, time_delta, 2);
}

static void
dump_1mem(struct prog_arg *a, double time_delta, long long cycle_delta)
{
    dump_throughput(a, time_delta, 1);
}

static void
memcpy_test(struct prog_arg *pa, struct thread_data *ta)
{
    int n = pa->nloop;
    int i;

    unsigned char *src = pa->mem1 + (ta->tid * pa->block_size);
    unsigned char *dst = pa->mem2 + (ta->tid * pa->block_size);
    int bs = pa->block_size_op;

    for (i=0; i<n; i++) {
        memcpy(dst, src, bs);
    }
}

static void
memset_test(struct prog_arg *pa, struct thread_data *ta)
{
    int n = pa->nloop;
    int i;

    unsigned char *dst = pa->mem1 + (ta->tid * pa->block_size);
    int bs = pa->block_size_op;

    for (i=0; i<n; i++) {
        memset(dst, 0x80, bs);
    }
}

#ifdef __x86_64__
#include "bench-x86_64.h"
#endif

#ifdef __arm__
#include "bench-arm.h"
#endif

struct bench {
#define SINGLE_THREAD (1<<0)
    int flags;

    const char *name;
    test_func_t func;
    dump_func_t dump;
};

struct bench bench_list[] = {
    {0, "memcpy", memcpy_test, dump_2mem},
    {0, "memset", memset_test, dump_1mem},

#ifdef __x86_64__
    {0, "ntdqa", ntdqa_test, dump_1mem},
    {0, "sse_store", sse_store_test, dump_1mem},
    {0, "sse_load", sse_load_test, dump_1mem},
    {0, "sse_copy", sse_copy_test, dump_2mem},
    {0, "sse_copy_ntdqa", sse_copy_ntdqa_test, dump_2mem},
    {SINGLE_THREAD, "rm_op", rm_op, NULL},
#endif

#ifdef __arm__
    {0, "neon_store", neon_store_test, dump_1mem},
    {0, "neon_copy", neon_copy_test, dump_2mem},
    {SINGLE_THREAD, "a15-ipc3", a15_ipc3, NULL},
    {SINGLE_THREAD, "int_latency", int_latency, NULL},
    {SINGLE_THREAD, "fadd_latency", fadd_latency, NULL},
    {SINGLE_THREAD, "fmul_latency", fmul_latency, NULL},
    {SINGLE_THREAD, "fma_latency", fma_latency, NULL},
    {SINGLE_THREAD, "fma_throughput", fma_throughput, NULL},
#endif

};

#define NUM_BENCH (sizeof(bench_list)/sizeof(bench_list[0]))

static void *
bench_start(void *p)
{
    struct thread_data *a = (struct thread_data*)p;

    a->bench->func(a->prog_arg, a);

    return NULL;
}

static void
run_bench(struct thread_data *d, struct bench *b, struct prog_arg *a)
{
    pthread_t t;

    d->bench = b;
    d->prog_arg = a;

    pthread_create(&t, NULL, bench_start, d);

    d->t  = t;
}

static void
usage(void) {
    puts("mem [-n nloop] [-b block_size] [-t num_thread] [-o op_name]");
    exit(0);
}



int
main(int argc, char **argv)
{

    int block_size = 1024;
    int nloop = 1024;
    int num_thread = 2;
    int opt, ti;
    int test_id = 0;
    struct prog_arg prog_arg;
    struct thread_data *thread_data;
    size_t mem_size, a_block_size;
    int verbose = 0;
    double tb, te;
    long long cb, ce;

    while ((opt = getopt(argc, argv, "n:b:t:o:v")) != -1) {
        switch (opt) {
        case 'n':
            nloop = atoi(optarg);
            break;

        case 'b':
            block_size = atoi(optarg);
            break;

        case 't':
            num_thread = atoi(optarg);
            break;

        case 'o': {
            int i;
            for (i=0; i<NUM_BENCH; i++) {
                if (strcmp(bench_list[i].name, optarg) == 0) {
                    test_id = i;
                    break;
                }
            }

            if (i == NUM_BENCH) {
                printf("invalid test name : %s\n", optarg);
                usage();
            }
        }
            break;

        case 'v':
            verbose = 1;
            break;

        default:
            usage();
            break;
        }
    }

    thread_data = malloc(sizeof(*thread_data) * num_thread);

    a_block_size = ALIGN_UP(block_size, 64);
    prog_arg.block_size_op = block_size;
    prog_arg.block_size = a_block_size;
    prog_arg.nloop = nloop;
    prog_arg.nthread = num_thread;

    if (verbose) {
        printf("op=%s, nloop=%d, block_size=%d, num_thread=%d\n",
               bench_list[test_id].name,
               nloop,
               block_size,
               num_thread);
    }

    mem_size = a_block_size * num_thread;

    prog_arg.mem1 = memalign(64, mem_size);
    prog_arg.mem2 = memalign(64, mem_size);
    prog_arg.mem3 = memalign(64, mem_size);

    memset(prog_arg.mem1, 0xff, mem_size);
    memset(prog_arg.mem2, 0xff, mem_size);
    memset(prog_arg.mem3, 0xff, mem_size);

    if (bench_list[test_id].flags & SINGLE_THREAD) {
        bench_list[test_id].func(&prog_arg, NULL);
    } else {
        for (ti=0; ti<num_thread; ti++) {
            thread_data[ti].tid = ti;
        }

        tb = get_sec();
        for (ti=0; ti<num_thread; ti++) {
            run_bench(&thread_data[ti], &bench_list[test_id], &prog_arg);
        }

        for (ti=0; ti<num_thread; ti++) {
            pthread_join(thread_data[ti].t, NULL);
        }
        te = get_sec();

        bench_list[test_id].dump(&prog_arg, te-tb, 0);
    }
}
