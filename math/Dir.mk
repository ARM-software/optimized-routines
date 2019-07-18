# Makefile fragment - requires GNU make
#
# Copyright (c) 2019, Arm Limited.
# SPDX-License-Identifier: MIT

math-lib-srcs := $(wildcard $(srcdir)/math/*.[cS])
math-test-srcs := \
	$(srcdir)/math/test/mathtest.c \
	$(srcdir)/math/test/mathbench.c \
	$(srcdir)/math/test/ulp.c \

math-test-host-srcs := $(wildcard $(srcdir)/math/test/rtest/*.[cS])
math-includes-src := $(wildcard $(srcdir)/math/include/*.h)
math-includes := $(math-includes-src:$(srcdir)/math/%=build/%)

math-libs := \
	build/lib/libmathlib.so \
	build/lib/libmathlib.a \

math-tools := \
	build/bin/mathtest \
	build/bin/mathbench \
	build/bin/mathbench_libc \
	build/bin/runulp.sh \
	build/bin/ulp \

math-host-tools := \
	build/bin/rtest \

math-lib-base := $(basename $(math-lib-srcs))
math-lib-objs := $(math-lib-base:$(srcdir)/%=build/%.o)
math-test-base := $(basename $(math-test-srcs))
math-test-objs := $(math-test-base:$(srcdir)/%=build/%.o)
math-test-host-base := $(basename $(math-test-host-srcs))
math-test-host-objs := $(math-test-host-base:$(srcdir)/%=build/%.o)

math-objs := \
	$(math-lib-objs) \
	$(math-test-objs) \
	$(math-test-host-objs) \

all-math: $(math-libs) $(math-tools) $(math-includes)

TESTS = $(wildcard $(srcdir)/math/test/testcases/directed/*.tst)
RTESTS = $(wildcard $(srcdir)/math/test/testcases/random/*.tst)

$(math-objs) $(math-objs:%.o=%.os): $(math-includes)
build/math/test/mathtest.o: CFLAGS_ALL += -fmath-errno
$(math-test-host-objs): CC = $(HOST_CC)
$(math-test-host-objs): CFLAGS_ALL = $(HOST_CFLAGS)

build/math/test/ulp.o: $(srcdir)/math/test/ulp.h

build/lib/libmathlib.so: $(math-lib-objs:%.o=%.os)
	$(CC) $(CFLAGS_ALL) $(LDFLAGS_ALL) -shared -o $@ $^

build/lib/libmathlib.a: $(math-lib-objs)
	rm -f $@
	$(AR) rc $@ $^
	$(RANLIB) $@

$(math-host-tools): HOST_LDLIBS += -lm -lmpfr -lmpc
$(math-tools): LDLIBS += -lm

build/bin/rtest: $(math-test-host-objs)
	$(HOST_CC) $(HOST_CFLAGS) $(HOST_LDFLAGS) -o $@ $^ $(HOST_LDLIBS)

build/bin/mathtest: build/math/test/mathtest.o build/lib/libmathlib.a
	$(CC) $(CFLAGS_ALL) $(LDFLAGS_ALL) -static -o $@ $^ $(LDLIBS)

build/bin/mathbench: build/math/test/mathbench.o build/lib/libmathlib.a
	$(CC) $(CFLAGS_ALL) $(LDFLAGS_ALL) -static -o $@ $^ $(LDLIBS)

build/bin/mathbench_libc: build/math/test/mathbench.o
	$(CC) $(CFLAGS_ALL) $(LDFLAGS_ALL) -static -o $@ $^ $(LDLIBS)

build/bin/ulp: build/math/test/ulp.o build/lib/libmathlib.a
	$(CC) $(CFLAGS_ALL) $(LDFLAGS_ALL) -static -o $@ $^ $(LDLIBS)

build/include/%.h: $(srcdir)/math/include/%.h
	cp $< $@

build/bin/%.sh: $(srcdir)/math/test/%.sh
	cp $< $@

check-math-test: $(math-tools)
	cat $(TESTS) | $(EMULATOR) build/bin/mathtest

check-math-rtest: $(math-host-tools) $(math-tools)
	cat $(RTESTS) | build/bin/rtest | $(EMULATOR) build/bin/mathtest

check-math-ulp: $(math-tools)
	build/bin/runulp.sh $(EMULATOR)

check-math: check-math-test check-math-rtest check-math-ulp

.PHONY: all-math check-math-test check-math-rtest check-math-ulp check-math
