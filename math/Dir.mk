# Makefile fragment - requires GNU make
#
# Copyright (c) 2019-2024, Arm Limited.
# SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception

ifneq ($(OS),Linux)
  ifeq ($(WANT_SIMD_EXCEPT),1)
    $(error WANT_SIMD_EXCEPT is not supported outside Linux)
  endif
  ifneq ($(USE_MPFR),1)
    $(warning WARNING: Double-precision ULP tests will not be usable without MPFR)
  endif
  ifeq ($(USE_GLIBC_ABI),1)
    $(error Can only generate special GLIBC symbols on Linux - please disable USE_GLIBC_ABI)
  endif
endif

S := $(srcdir)/math
B := build/math

math-lib-srcs := $(wildcard $(S)/*.[cS])
ifeq ($(OS),Linux)
# Vector symbols only supported on Linux
math-lib-srcs += $(wildcard $(S)/$(ARCH)/*.[cS])
endif

math-test-srcs := \
	$(S)/test/mathtest.c \
	$(S)/test/mathbench.c \
	$(S)/test/ulp.c \

math-test-host-srcs := $(wildcard $(S)/test/rtest/*.[cS])

math-includes := $(patsubst $(S)/%,build/%,$(wildcard $(S)/include/*.h))
math-test-includes := $(patsubst $(S)/%,build/include/%,$(wildcard $(S)/test/*.h))

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

math-lib-objs := $(patsubst $(S)/%,$(B)/%.o,$(basename $(math-lib-srcs)))
math-test-objs := $(patsubst $(S)/%,$(B)/%.o,$(basename $(math-test-srcs)))
math-host-objs := $(patsubst $(S)/%,$(B)/%.o,$(basename $(math-test-host-srcs)))
math-target-objs := $(math-lib-objs) $(math-test-objs)
math-objs := $(math-target-objs) $(math-target-objs:%.o=%.os) $(math-host-objs)

math-files := \
	$(math-objs) \
	$(math-libs) \
	$(math-tools) \
	$(math-host-tools) \
	$(math-includes) \
	$(math-test-includes) \

all-math: $(math-libs) $(math-tools) $(math-includes) $(math-test-includes)

$(math-objs): $(math-includes) $(math-test-includes)
$(math-objs): CFLAGS_ALL += $(math-cflags)
$(B)/test/mathtest.o: CFLAGS_ALL += -fmath-errno
$(math-host-objs): CC = $(HOST_CC)
$(math-host-objs): CFLAGS_ALL = $(HOST_CFLAGS)

$(B)/test/ulp.o: $(S)/test/ulp.h

build/lib/libmathlib.so: $(math-lib-objs:%.o=%.os)
	$(CC) $(CFLAGS_ALL) $(LDFLAGS) -shared -o $@ $^

build/lib/libmathlib.a: $(math-lib-objs)
	rm -f $@
	$(AR) rc $@ $^
	$(RANLIB) $@

$(math-host-tools): HOST_LDLIBS += $(libm-libs) $(mpfr-libs) $(mpc-libs)
$(math-tools): LDLIBS += $(math-ldlibs) $(libm-libs)

# math-sve-cflags should be empty if WANT_SVE_MATH is not enabled
$(math-tools): CFLAGS_ALL += $(math-sve-cflags)
ifneq ($(OS),Darwin)
  $(math-tools): LDFLAGS += -static
endif

build/bin/rtest: $(math-host-objs)
	$(HOST_CC) $(HOST_CFLAGS) $(HOST_LDFLAGS) -o $@ $^ $(HOST_LDLIBS)

build/bin/mathtest: $(B)/test/mathtest.o build/lib/libmathlib.a
	$(CC) $(CFLAGS_ALL) $(LDFLAGS) -o $@ $^ $(libm-libs)

build/bin/mathbench: $(B)/test/mathbench.o build/lib/libmathlib.a
	$(CC) $(CFLAGS_ALL) $(LDFLAGS) -o $@ $^ $(libm-libs)

# This is not ideal, but allows custom symbols in mathbench to get resolved.
build/bin/mathbench_libc: $(B)/test/mathbench.o build/lib/libmathlib.a
	$(CC) $(CFLAGS_ALL) $(LDFLAGS) -o $@ $< $(libm-libs) $(libc-libs) build/lib/libmathlib.a $(libm-libs)

build/bin/ulp: $(B)/test/ulp.o build/lib/libmathlib.a
	$(CC) $(CFLAGS_ALL) $(LDFLAGS) -o $@ $^ $(LDLIBS)

build/include/%.h: $(S)/include/%.h
	cp $< $@

build/include/test/%.h: $(S)/test/%.h
	cp $< $@

build/bin/%.sh: $(S)/test/%.sh
	cp $< $@

math-tests := $(wildcard $(S)/test/testcases/directed/*.tst)
ifneq ($(WANT_EXP10_TESTS),1)
math-tests := $(filter-out %exp10.tst, $(math-tests))
endif
math-rtests := $(wildcard $(S)/test/testcases/random/*.tst)

check-math-test: $(math-tools)
	cat $(math-tests) | $(EMULATOR) build/bin/mathtest $(math-testflags)

check-math-rtest: $(math-host-tools) $(math-tools)
	cat $(math-rtests) | build/bin/rtest | $(EMULATOR) build/bin/mathtest $(math-testflags)

check-math-ulp: $(math-tools)
	ULPFLAGS="$(math-ulpflags)" WANT_SIMD_TESTS="$(WANT_SIMD_TESTS)" WANT_SIMD_EXCEPT="$(WANT_SIMD_EXCEPT)" WANT_EXP10_TESTS="$(WANT_EXP10_TESTS)" build/bin/runulp.sh $(EMULATOR)

check-math: check-math-test check-math-rtest check-math-ulp

install-math: \
 $(math-libs:build/lib/%=$(DESTDIR)$(libdir)/%) \
 $(math-includes:build/include/%=$(DESTDIR)$(includedir)/%)

clean-math:
	rm -f $(math-files)

.PHONY: all-math check-math-test check-math-rtest check-math-ulp check-math install-math clean-math
