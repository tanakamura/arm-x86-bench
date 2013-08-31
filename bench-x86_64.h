/* -*- indent-tabs-mode : nil; c-basic-offset : 4 -*- */

#include <x86intrin.h>

static void
ntdqa_test(struct prog_arg *pa, struct thread_data *ta)
{
    int n = pa->nloop;
    int i;

    unsigned char *dst = pa->mem1 + (ta->tid * pa->block_size);
    int bs = pa->block_size_op;

    for (i=0; i<n; i++) {
        int niter = bs / (16 * 4);
        int bi;
        __m128i vzero = _mm_setzero_si128();

        for (bi=0; bi<niter; bi++) {
            _mm_stream_si128((__m128i*)&dst[(bi*4 + 0) * 16], vzero);
            _mm_stream_si128((__m128i*)&dst[(bi*4 + 1) * 16], vzero);
            _mm_stream_si128((__m128i*)&dst[(bi*4 + 2) * 16], vzero);
            _mm_stream_si128((__m128i*)&dst[(bi*4 + 3) * 16], vzero);
        }

        __asm__ __volatile__ ("":::"memory");
    }
}

static void
sse_store_test(struct prog_arg *pa, struct thread_data *ta)
{
    int n = pa->nloop;
    int i;

    unsigned char *dst = pa->mem1 + (ta->tid * pa->block_size);
    int bs = pa->block_size_op;

    for (i=0; i<n; i++) {
        int niter = bs / (16 * 4);
        int bi;
        __m128i vzero = _mm_setzero_si128();

        for (bi=0; bi<niter; bi++) {
            _mm_store_si128((__m128i*)&dst[(bi*4 + 0) * 16], vzero);
            _mm_store_si128((__m128i*)&dst[(bi*4 + 1) * 16], vzero);
            _mm_store_si128((__m128i*)&dst[(bi*4 + 2) * 16], vzero);
            _mm_store_si128((__m128i*)&dst[(bi*4 + 3) * 16], vzero);
        }

        __asm__ __volatile__ ("":::"memory");
    }
}


static void
sse_load_test(struct prog_arg *pa, struct thread_data *ta)
{
    int n = pa->nloop;
    int i;

    unsigned char *src = pa->mem1 + (ta->tid * pa->block_size);
    int bs = pa->block_size_op;

    for (i=0; i<n; i++) {
        int niter = bs / (16 * 4);
        int bi;
        __m128i vzero = _mm_setzero_si128();

        for (bi=0; bi<niter; bi++) {
            *(volatile __m128i*)&src[(bi*4 + 0) * 16];
            *(volatile __m128i*)&src[(bi*4 + 1) * 16];
            *(volatile __m128i*)&src[(bi*4 + 2) * 16];
            *(volatile __m128i*)&src[(bi*4 + 3) * 16];
        }

        __asm__ __volatile__ ("":::"memory");
    }
}


static void
sse_copy_test(struct prog_arg *pa, struct thread_data *ta)
{
    int n = pa->nloop;
    int i;

    unsigned char *src = pa->mem1 + (ta->tid * pa->block_size);
    unsigned char *dst = pa->mem2 + (ta->tid * pa->block_size);
    int bs = pa->block_size_op;

    for (i=0; i<n; i++) {
        int niter = bs / (16 * 4);
        int bi;
        __m128i vzero = _mm_setzero_si128();

        for (bi=0; bi<niter; bi++) {
            __m128i v0 = *(__m128i*)&src[(bi*4 + 0) * 16];
            __m128i v1 = *(__m128i*)&src[(bi*4 + 1) * 16];
            __m128i v2 = *(__m128i*)&src[(bi*4 + 2) * 16];
            __m128i v3 = *(__m128i*)&src[(bi*4 + 3) * 16];

            _mm_store_si128((__m128i*)&dst[(bi*4 + 0) * 16], v0);
            _mm_store_si128((__m128i*)&dst[(bi*4 + 1) * 16], v1);
            _mm_store_si128((__m128i*)&dst[(bi*4 + 2) * 16], v2);
            _mm_store_si128((__m128i*)&dst[(bi*4 + 3) * 16], v3);
        }

        __asm__ __volatile__ ("":::"memory");
    }
}

static void
sse_copy_ntdqa_test(struct prog_arg *pa, struct thread_data *ta)
{
    int n = pa->nloop;
    int i;

    unsigned char *src = pa->mem1 + (ta->tid * pa->block_size);
    unsigned char *dst = pa->mem2 + (ta->tid * pa->block_size);
    int bs = pa->block_size_op;

    for (i=0; i<n; i++) {
        int niter = bs / (16 * 4);
        int bi;
        __m128i vzero = _mm_setzero_si128();

        for (bi=0; bi<niter; bi++) {
            __m128i v0 = *(__m128i*)&src[(bi*4 + 0) * 16];
            __m128i v1 = *(__m128i*)&src[(bi*4 + 1) * 16];
            __m128i v2 = *(__m128i*)&src[(bi*4 + 2) * 16];
            __m128i v3 = *(__m128i*)&src[(bi*4 + 3) * 16];

            _mm_stream_si128((__m128i*)&dst[(bi*4 + 0) * 16], v0);
            _mm_stream_si128((__m128i*)&dst[(bi*4 + 1) * 16], v1);
            _mm_stream_si128((__m128i*)&dst[(bi*4 + 2) * 16], v2);
            _mm_stream_si128((__m128i*)&dst[(bi*4 + 3) * 16], v3);
        }

        __asm__ __volatile__ ("":::"memory");
    }
}


static void
rm_op(struct prog_arg *pa, struct thread_data *ta)
{
    int n = pa->nloop;
    int i;
    long long tb;
    long long te;
    long long niter = 4096*128;
    void *ptr = pa->mem1;

    time_init();

    tb = get_cycle();

    for (i=0; i<n; i++) {
        long long bi = 0;
        
        __asm__ __volatile__ ("xorq %%r10, %%r10\n\t"
                              "xorq %%r12, %%r12\n\t"
                              "mov %2, %%r13\n\t"
                              ".align 4\n\t"
                              "1:\n\t"

                              ITER16("add 16384(%2,%%r12,2), %%r8\n\t"
                                     "movq %%r11, 16384+32(%%r13,%%r10,2)\n\t")

                              "add $1, %0\n\t"
                              "cmpq %0, %1\n\t"
                              "jne 1b\n\t"

                              :"+r"(bi)
                              :"r"(niter), "r"(ptr)
                              :"memory", "r8", "r9", "r10", "r11", "r12", "r13"
                              );
    }

    te = get_cycle();

    time_end();

    double inst_total = n * niter * (double)(32 + 3);

    printf("IPC=%f %lld\n", (double)inst_total/(double)(te-tb), te-tb);
}
