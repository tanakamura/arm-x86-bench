/* -*- indent-tabs-mode : nil; c-basic-offset : 4 -*- */

#define _GNU_SOURCE

#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>


#include "cycle.h"

struct Base {
    virtual void f() = 0;
};
#define GENCLASS(name)                          \
    struct name : public Base { virtual void f(){} };

GENCLASS(A);GENCLASS(B);GENCLASS(C);GENCLASS(D);GENCLASS(E);GENCLASS(F);

int main()
{
#define NOBJ 6

    time_init();

    Base *a[][NOBJ] = {
#ifdef HOGE
        {new A(), new A(), new A(), new A(), new A(), new A()}, // 全ヒット    : Sandy 42
        {new A(), new B(), new A(), new A(), new A(), new A()}, // 一個だけ違う: Sandy 42
        {new A(), new B(), new A(), new A(), new B(), new A()}, // ふたつ違う  : Sandy 55-65??
#endif
        {new A(), new B(), new C(), new D(), new E(), new F()}, // 全部違う    : Sandy(-DHOGE) 114, Sandy(-DHOGE無し) 95
        {new A(), new E(), new C(), new A(), new E(), new F()}, // 全部違う    : Sandy(-DHOGE) 140, Sandy(-DHOGE無し) 37
    };

    int npattern = sizeof(a)/sizeof(a[0]);

    for (int pat=0; pat<npattern; pat++) {
        for (int iter=0; iter<8; iter++) {
            int nloop = 1024*1024*8;
            long long b = get_cycle();
            Base **tbl = a[pat];
            for (int i=0; i<nloop; i++) {
                for (int j=0; j<NOBJ; j++) {
                    tbl[j]->f();
                }
            }
            long long e = get_cycle();

            printf("pattern:%d iter:%d %f\n", pat, iter, (e-b)/(double)nloop);
        }
    }

    time_end();
}
