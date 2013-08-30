#include <asm/unistd.h>
#include <linux/perf_event.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

static int fddev = -1;
static inline void
time_init(void)
{
    static struct perf_event_attr attr;
    attr.type = PERF_TYPE_HARDWARE;
    attr.config = PERF_COUNT_HW_CPU_CYCLES;
    fddev = syscall(__NR_perf_event_open, &attr, 0, -1, -1, 0);

    printf("%d\n", fddev);
}

static long long
get_cycle()
{
	/*
	  long long result = 0;
    if (read(fddev, &result, sizeof(result)) != sizeof(long long)) {
	    perror("read ev");
        exit(1);
    }
    return result;
	*/

	int result;

	asm volatile ("MRC p15, 0, %0, c9, c13, 0\t\n": "=r"(result));

	return result;
}


int main()
{
	long long t0, t1;
	time_init();

	t0 = get_cycle();
	sleep(2);
	t1 = get_cycle();

	printf("%lld\n", t1-t0);
				 
}
