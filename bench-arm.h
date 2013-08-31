/* -*- indent-tabs-mode : nil; c-basic-offset : 4 -*- */

#include <arm_neon.h>

static void
neon_store_test(struct prog_arg *pa, struct thread_data *ta)
{
    int n = pa->nloop;
    int i;

    unsigned int bs = pa->block_size_op;

    for (i=0; i<n; i++) {
        int niter = bs / (16U * 4U);
        int bi;
        unsigned char *dst = pa->mem1 + (ta->tid * pa->block_size);

        bi = 0;
        __asm__ __volatile__ ("1:\n\t"
                              "adds %0, %0, #1\n\t"
                              "cmp %0, %2\n\t"
                              "vst1.8 {d0,d1}, [%1]!\n\t"
                              "vst1.8 {d0,d1}, [%1]!\n\t"
                              "vst1.8 {d0,d1}, [%1]!\n\t"
                              "vst1.8 {d0,d1}, [%1]!\n\t"
                              "bne 1b\n\t"
                              :"+r"(bi),
                               "+r"(dst)
                              :"r"(niter)
                              :"q0", "memory");

        __asm__ __volatile__ ("":::"memory");
    }
}

static void
neon_load_test(struct prog_arg *pa, struct thread_data *ta)
{
    int n = pa->nloop;
    int i;

    unsigned int bs = pa->block_size_op;

    for (i=0; i<n; i++) {
        int niter = bs / (16U * 4U);
        int bi;
        unsigned char *src = pa->mem1 + (ta->tid * pa->block_size);

        bi = 0;
        __asm__ __volatile__ ("1:\n\t"
                              "adds %0, %0, #1\n\t"
                              "cmp %0, %2\n\t"
                              "vld1.8 {d0,d1}, [%1]!\n\t"
                              "vld1.8 {d0,d1}, [%1]!\n\t"
                              "vld1.8 {d0,d1}, [%1]!\n\t"
                              "vld1.8 {d0,d1}, [%1]!\n\t"
                              "bne 1b\n\t"
                              :"+r"(bi),
                               "+r"(src)
                              :"r"(niter)
                              :"q0", "memory");

        __asm__ __volatile__ ("":::"memory");
    }
}

static void
neon_copy_test(struct prog_arg *pa, struct thread_data *ta)
{
    int n = pa->nloop;
    int i;

    unsigned int bs = pa->block_size_op;

    for (i=0; i<n; i++) {
        int niter = bs / (16U * 4U);
        int bi;
        unsigned char *dst = pa->mem1 + (ta->tid * pa->block_size);
        unsigned char *src = pa->mem2 + (ta->tid * pa->block_size);

        bi = 0;
        __asm__ __volatile__ ("1:\n\t"
                              "adds %0, %0, #1\n\t"
                              "cmp %0, %3\n\t"
                              "vld1.8 {d0,d1}, [%2]!\n\t"
                              "vst1.8 {d0,d1}, [%1]!\n\t"
                              "vld1.8 {d0,d1}, [%2]!\n\t"
                              "vst1.8 {d0,d1}, [%1]!\n\t"
                              "vld1.8 {d0,d1}, [%2]!\n\t"
                              "vst1.8 {d0,d1}, [%1]!\n\t"
                              "vld1.8 {d0,d1}, [%2]!\n\t"
                              "vst1.8 {d0,d1}, [%1]!\n\t"
                              "bne 1b\n\t"
                              :"+r"(bi),
                               "+r"(dst),
                               "+r"(src)
                              :"r"(niter)
                              :"q0", "memory");

        __asm__ __volatile__ ("":::"memory");
    }
}

static void
a15_ipc3(struct prog_arg *pa, struct thread_data *ta)
{
    int n = pa->nloop;
    int i;
    long long tb;
    long long te;
    int niter = 4096*128;
    void *ptr = pa->mem1;

    time_init();

    tb = get_cycle();

    for (i=0; i<n; i++) {

        int bi = 0;
        __asm__ __volatile__ ("vsub.f32 d0, d0, d0\n\t"
                              "vsub.f32 d1, d1, d1\n\t"
                              "vsub.f32 d2, d2, d2\n\t"
                              "vsub.f32 d3, d3, d3\n\t"
                              "vsub.f32 d4, d4, d4\n\t"
                              "vsub.f32 d5, d5, d5\n\t"
                              "vsub.f32 d6, d6, d6\n\t"

                              "1:\n\t"
#if 0
                              "adds r7, r7, r7\n\t"
                              "vadd.u32 d1, d0, d0\n\t"
                              //"vmul.f32 d3, d0, d0\n\t"
                              "vadd.u32 d1, d0, d0\n\t"

                              "adds r7, r7, r7\n\t"
                              "vmul.f32 d3, d0, d0\n\t"
                              "vmul.f32 d1, d0, d0\n\t"

                              "adds r7, r7, r7\n\t"
                              "vadd.f32 d3, d0, d0\n\t"
                              "vfma.f32 d1, d0, d0\n\t"
#else
                              "adds r7, r7, r7\n\t"
                              "vadd.u32 q2, q0, q0\n\t"
                              "vadd.u32 q1, q0, q0\n\t"

                              "adds r7, r7, r7\n\t"
                              "vadd.u32 q2, q0, q0\n\t"
                              "vadd.u32 q1, q0, q0\n\t"

                              "adds r7, r7, r7\n\t"
                              "adds r8, r8, r8\n\t"
                              "vadd.u32 d2, d0, d0\n\t"
#endif

                              "adds %0, %0, #1\n\t"
                              "cmp %0, %1\n\t"
                              "bne 1b"

                              :"+r"(bi)
                              :"r"(niter), "r"(ptr)
                              :"memory", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11", "r12", "d0", "d1", "d2", "d3", "d4", "d5", "d6"
                              );
    }

    te = get_cycle();

    time_end();

    double inst_total = n * niter * (double)12;

    printf("IPC=%f %lld\n", inst_total / (double)(te-tb), te-tb);
}

static void
int_latency(struct prog_arg *pa, struct thread_data *ta)
{
    int n = pa->nloop;
    int i;
    long long tb;
    long long te;
    int niter = 4096*128;
    void *ptr = pa->mem1;

    time_init();

    tb = get_cycle();

    for (i=0; i<n; i++) {

        int bi = 0;
        __asm__ __volatile__ (".align 4\n\t"
                              "1:\n\t"

                              ITER16("adds r8, #1\n\t")

                              "adds %0, %0, #1\n\t"
                              "cmp %0, %1\n\t"
                              "bne 1b"

                              :"+r"(bi)
                              :"r"(niter), "r"(ptr)
                              :"memory", "r8"
                              );
    }

    te = get_cycle();

    time_end();

    double inst_total = n * niter * (double)19;

    printf("IPC=%f %lld\n", inst_total / (double)(te-tb), te-tb);
}

static void
fadd_latency(struct prog_arg *pa, struct thread_data *ta)
{
    int n = pa->nloop;
    int i;
    long long tb;
    long long te;
    int niter = 4096*128;
    void *ptr = pa->mem1;

    time_init();

    tb = get_cycle();

    for (i=0; i<n; i++) {

        int bi = 0;
        __asm__ __volatile__ ("vsub.f32 q0, q0, q0\n\t"
                              ".align 4\n\t"
                              "1:\n\t"

                              ITER16("vadd.f32 q0, q0\n\t")

                              "adds %0, %0, #1\n\t"
                              "cmp %0, %1\n\t"
                              "bne 1b"

                              :"+r"(bi)
                              :"r"(niter), "r"(ptr)
                              :"memory", "r8"
                              );
    }

    te = get_cycle();

    time_end();

    double inst_total = n * niter * (double)16;

    printf("CPI=%f %lld\n", (double)(te-tb)/(double)inst_total, te-tb);
}

static void
fmul_latency(struct prog_arg *pa, struct thread_data *ta)
{
    int n = pa->nloop;
    int i;
    long long tb;
    long long te;
    int niter = 4096*128;
    void *ptr = pa->mem1;

    time_init();

    tb = get_cycle();

    for (i=0; i<n; i++) {
        int bi = 0;
        __asm__ __volatile__ ("vsub.f32 q0, q0, q0\n\t"
                              ".align 4\n\t"
                              "1:\n\t"

                              ITER16("vmul.f32 q0, q0\n\t")

                              "adds %0, %0, #1\n\t"
                              "cmp %0, %1\n\t"
                              "bne 1b"

                              :"+r"(bi)
                              :"r"(niter), "r"(ptr)
                              :"memory", "r8"
                              );
    }

    te = get_cycle();

    time_end();

    double inst_total = n * niter * (double)16;

    printf("CPI=%f %lld\n", (double)(te-tb)/(double)inst_total, te-tb);
}

static void
fma_latency(struct prog_arg *pa, struct thread_data *ta)
{
    int n = pa->nloop;
    int i;
    long long tb;
    long long te;
    int niter = 4096*128;
    void *ptr = pa->mem1;

    time_init();

    tb = get_cycle();

    for (i=0; i<n; i++) {
        int bi = 0;

        
        __asm__ __volatile__ ("vsub.f32 q0, q0, q0\n\t"
                              ".align 4\n\t"
                              "1:\n\t"

                              ITER16("vfma.f32 q0, q0\n\t")

                              "adds %0, %0, #1\n\t"
                              "cmp %0, %1\n\t"
                              "bne 1b"

                              :"+r"(bi)
                              :"r"(niter), "r"(ptr)
                              :"memory", "r8", "q0", "q1", "q2", "q3", "q4", "q5", "q6", "q7", "q8"
                              );
    }

    te = get_cycle();

    time_end();

    double inst_total = n * niter * (double)16;

    printf("CPI=%f %lld\n", (double)(te-tb)/(double)inst_total, te-tb);
}

static void
fma_throughput(struct prog_arg *pa, struct thread_data *ta)
{
    int n = pa->nloop;
    int i;
    long long tb;
    long long te;
    int niter = 4096*128;
    void *ptr = pa->mem1;

    time_init();

    tb = get_cycle();

    for (i=0; i<n; i++) {
        int bi = 0;

        
        __asm__ __volatile__ ("vsub.f32 q0, q0, q0\n\t"
                              ".align 4\n\t"
                              "1:\n\t"

                              ITER4("vfma.f32 q0, q0\n\t"
                                    "vfma.f32 q1, q1\n\t"
                                    "vfma.f32 q2, q2\n\t"
                                    "vfma.f32 q3, q3\n\t"
                                    "vfma.f32 q4, q4\n\t"
                                    "vfma.f32 q5, q5\n\t"
                                    "vfma.f32 q6, q6\n\t"
                                    "vfma.f32 q7, q7\n\t")

                              "adds %0, %0, #1\n\t"
                              "cmp %0, %1\n\t"
                              "bne 1b"

                              :"+r"(bi)
                              :"r"(niter), "r"(ptr)
                              :"memory", "r8", "q0", "q1", "q2", "q3", "q4", "q5", "q6", "q7", "q8"
                              );
    }

    te = get_cycle();

    time_end();

    double inst_total = n * niter * (double)32;

    printf("IPC=%f %lld\n", (double)inst_total/(double)(te-tb), te-tb);
}


static void
block_branch(struct prog_arg *pa, struct thread_data *ta)
{
    int n = pa->nloop;
    int i;
    long long tb;
    long long te;
    int niter = 4096*128;
    void *ptr = pa->mem1;

    time_init();

    tb = get_cycle();

    for (i=0; i<n; i++) {
        int bi = 0;

        
        __asm__ __volatile__ ("vsub.f32 q0, q0, q0\n\t"
                              "mov r3, #0\n\t"
                              ".align 4\n\t"
                              "1:\n\t"

                              ITER16("cmp r3, #1\n\t"
                                     "beq.n 2f\n\t"
                                     ".align 2\n\t"
                                     "2:\n\t"
                                    )

                              "adds %0, %0, #1\n\t"
                              "cmp %0, %1\n\t"
                              "bne 1b"

                              :"+r"(bi)
                              :"r"(niter), "r"(ptr)
                              :"memory", "r3", "r4", "r8", "q0", "q1", "q2", "q3", "q4", "q5", "q6", "q7", "q8"
                              );
    }

    te = get_cycle();

    time_end();

    double inst_total = n * niter * (double)35;

    printf("IPC=%f %lld\n", (double)inst_total/(double)(te-tb), te-tb);
}
