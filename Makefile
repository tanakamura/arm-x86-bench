MACHINE=$(shell uname -m)
all: bench-$(MACHINE)

CFLAGS = -O2 -save-temps

LDLIBS = -lpthread -lrt 
LDFLAGS = 

ifeq "x86_64" "$(MACHINE)"
CFLAGS += 
else
CFLAGS += -mcpu=cortex-a15 -mfloat-abi=softfp -mfpu=neon-vfpv4
endif

bench-$(MACHINE): bench.c bench-x86_64.h bench-arm.h
	echo $(MACHINE)
	gcc $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDLIBS) 

clean: 
	rm -f bench-$(MACHINE)
