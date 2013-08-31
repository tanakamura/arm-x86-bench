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
dump_f32op1(struct prog_arg *a, double time_delta, long long cycle_delta)
{
    double num_op = (a->block_size/4) * (double)a->nloop * (double)a->nthread;

    num_op /= (time_delta*1000*1000);

    printf("%f [Mops/sec]\n", num_op);
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

static void
mem_reorder(struct prog_arg *pa, struct thread_data *ta)
{
    int n = pa->nloop;
    int i;
    long long tb;
    long long te;
    long long t1, t2;
    long long niter = 512;
    volatile unsigned int *ptr0 = (unsigned int*)pa->mem1;
    unsigned int *ptr1 = (unsigned int*)pa->mem1;

    asm volatile ("":"+r"(ptr0),"+r"(ptr1));

    ptr1 ++;

    time_init();

    tb = get_cycle();

    int sum0 = 0;
    for (i=0; i<n; i++) {
        long long bi = 0;
        int j;

        for (j=0; j<niter; j+=4) {
            ptr1[(j*4+0)*2] = 0;
            ptr0[(j*4+0)*2];
            ptr1[(j*4+1)*2] = 0;
            ptr0[(j*4+1)*2];
            ptr1[(j*4+2)*2] = 0;
            ptr0[(j*4+2)*2];
            ptr1[(j*4+3)*2] = 0;
            ptr0[(j*4+3)*2];
        }
    }
    te = get_cycle();

    t1 = te-tb;


    tb = get_cycle();

    for (i=0; i<n; i++) {
        long long bi = 0;
        int j;

        for (j=0; j<niter; j+=4) {
            ptr1[(j*4+0)*2] = 0;
            ptr1[(j*4+1)*2] = 0;
            ptr1[(j*4+2)*2] = 0;
            ptr1[(j*4+3)*2] = 0;

            ptr0[(j*4+0)*2];
            ptr0[(j*4+1)*2];
            ptr0[(j*4+2)*2];
            ptr0[(j*4+3)*2];
        }
    }
    te = get_cycle();

    t2 = te-tb;


    time_end();

    double inst_total = n * niter * 2.0;

    printf("IPC=%f/%f sum=%d\n",
           (double)inst_total/(double)(t1),
           (double)inst_total/(double)(t2),
           sum0);
}

static void
vecadd_test(struct prog_arg *p,
            struct thread_data *a)
{
    int bs = p->block_size_op;
    int nelem = bs/4, li, ei;
    int nloop = p->nloop;


    for (li=0; li<nloop; li++) {
        int* src0 = (int*)(p->mem1 + (a->tid * p->block_size));
        int* src1 = (int*)(p->mem3 + (a->tid * p->block_size));
        int* dst0 = (int*)(p->mem2 + (a->tid * p->block_size));

        for (ei=0; ei<nelem; ei++) {
            *(dst0++) = *(src0++) + *(src1++);
        }

        asm volatile ("":::"memory");
    }
}

static __attribute__((noinline)) void
recfunc0(int i)
{
    asm volatile ("":::"memory");
}
#ifdef __arm__
#define JMPRET asm volatile ("b 1f\n\tnop\n\t1:\n\tnop":::"memory", "r4", "r5"); 
#else
#define JMPRET asm volatile ("jmp 1f\n\tnop\n\t1:":::"memory"); 
#endif

#define GENR(n,n2)                              \
    static __attribute__((noinline)) void       \
    recfunc##n(int i)                           \
    {                                           \
        if (i) {                                \
            recfunc##n2(!i);                    \
        } else {                                \
            recfunc##n(!i);                     \
        }                                       \
        JMPRET;                                 \
    }


GENR(1,0)  GENR(2,1)  GENR(3,2)  GENR(4,3)  GENR(5,4)  GENR(6,5)  GENR(7,6)  GENR(8,7)  GENR(9,8)  GENR(10,9)  GENR(11,10)  GENR(12,11)  GENR(13,12)  GENR(14,13)  GENR(15,14)  GENR(16,15)  GENR(17,16)  GENR(18,17)  GENR(19,18)  GENR(20,19)  GENR(21,20)  GENR(22,21)  GENR(23,22)  GENR(24,23)  GENR(25,24)  GENR(26,25)  GENR(27,26)  GENR(28,27)  GENR(29,28)  GENR(30,29)  GENR(31,30)  GENR(32,31)  GENR(33,32)  GENR(34,33)  GENR(35,34)  GENR(36,35)  GENR(37,36)  GENR(38,37)  GENR(39,38)  GENR(40,39)  GENR(41,40)  GENR(42,41)  GENR(43,42)  GENR(44,43)  GENR(45,44)  GENR(46,45)  GENR(47,46)  GENR(48,47)  GENR(49,48)  GENR(50,49)  GENR(51,50)  GENR(52,51)  GENR(53,52)  GENR(54,53)  GENR(55,54)  GENR(56,55)  GENR(57,56)  GENR(58,57)  GENR(59,58)  GENR(60,59)  GENR(61,60)  GENR(62,61)  GENR(63,62)  ;
 
typedef void (*recfunc_t)(int);

static recfunc_t recfuncs[] = {
recfunc0, recfunc1, recfunc2, recfunc3, recfunc4, recfunc5, recfunc6, recfunc7, recfunc8, recfunc9, recfunc10, recfunc11, recfunc12, recfunc13, recfunc14, recfunc15, recfunc16, recfunc17, recfunc18, recfunc19, recfunc20, recfunc21, recfunc22, recfunc23, recfunc24, recfunc25, recfunc26, recfunc27, recfunc28, recfunc29, recfunc30, recfunc31, recfunc32, recfunc33, recfunc34, recfunc35, recfunc36, recfunc37, recfunc38, recfunc39, recfunc40, recfunc41, recfunc42, recfunc43, recfunc44, recfunc45, recfunc46, recfunc47, recfunc48, recfunc49, recfunc50, recfunc51, recfunc52, recfunc53, recfunc54, recfunc55, recfunc56, recfunc57, recfunc58, recfunc59, recfunc60, recfunc61, recfunc62, recfunc63
};

static void
return_addr_stack(struct prog_arg *p,
                  struct thread_data *a)
{
    int max_depth = 32;
    int md, li, n = p->nloop;

    time_init();

    for (md=1; md<max_depth; md++) {
        long long tb = get_cycle(), te;

        for (li=0; li<n; li++) {
            recfuncs[md](1);
        }

        te = get_cycle();

        printf("depth %d: %f\n", md, (te-tb)/(double)(n*md));
    }

    time_end();
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

    {SINGLE_THREAD, "mem_reorder", mem_reorder, NULL},

    {0, "vecadd", vecadd_test, dump_f32op1},
    {SINGLE_THREAD, "return_addr_stack", return_addr_stack, NULL},

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
    {0, "neon_load", neon_load_test, dump_1mem},
    {0, "neon_copy", neon_copy_test, dump_2mem},
    {SINGLE_THREAD, "a15-ipc3", a15_ipc3, NULL},
    {SINGLE_THREAD, "int_latency", int_latency, NULL},
    {SINGLE_THREAD, "fadd_latency", fadd_latency, NULL},
    {SINGLE_THREAD, "fmul_latency", fmul_latency, NULL},
    {SINGLE_THREAD, "fma_latency", fma_latency, NULL},
    {SINGLE_THREAD, "fma_throughput", fma_throughput, NULL},
    {SINGLE_THREAD, "block_branch", block_branch, NULL}
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
