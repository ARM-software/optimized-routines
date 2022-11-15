# Makefile fragment - requires GNU make
#
# Copyright (c) 2019-2022, Arm Limited.
# SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception

PLM := $(srcdir)/pl/math
AOR := $(srcdir)/math
B := build/pl/math

math-lib-srcs := $(wildcard $(PLM)/*.[cS])
math-test-srcs := \
	$(AOR)/test/mathtest.c \
	$(AOR)/test/mathbench.c \
	$(AOR)/test/ulp.c \

math-test-host-srcs := $(wildcard $(AOR)/test/rtest/*.[cS])

math-includes := $(patsubst $(PLM)/%,build/pl/%,$(wildcard $(PLM)/include/*.h))
math-test-includes := $(patsubst $(PLM)/%,build/pl/include/%,$(wildcard $(PLM)/test/*.h))

math-libs := \
	build/pl/lib/libmathlib.so \
	build/pl/lib/libmathlib.a \

math-tools := \
	build/pl/bin/mathtest \
	build/pl/bin/mathbench \
	build/pl/bin/mathbench_libc \
	build/pl/bin/runulp.sh \
	build/pl/bin/ulp \

math-host-tools := \
	build/pl/bin/rtest \

math-lib-objs := $(patsubst $(PLM)/%,$(B)/%.o,$(basename $(math-lib-srcs)))
math-test-objs := $(patsubst $(AOR)/%,$(B)/%.o,$(basename $(math-test-srcs)))
math-host-objs := $(patsubst $(AOR)/%,$(B)/%.o,$(basename $(math-test-host-srcs)))
math-target-objs := $(math-lib-objs) $(math-test-objs)
math-objs := $(math-target-objs) $(math-target-objs:%.o=%.os) $(math-host-objs)

pl/math-files := \
	$(math-objs) \
	$(math-libs) \
	$(math-tools) \
	$(math-host-tools) \
	$(math-includes) \
	$(math-test-includes) \

all-pl/math: $(math-libs) $(math-tools) $(math-includes) $(math-test-includes)

$(math-objs): $(math-includes) $(math-test-includes)
$(math-objs): CFLAGS_PL += $(math-cflags)
$(B)/test/mathtest.o: CFLAGS_PL += -fmath-errno
$(math-host-objs): CC = $(HOST_CC)
$(math-host-objs): CFLAGS_PL = $(HOST_CFLAGS)

$(B)/test/ulp.o: $(AOR)/test/ulp.h

build/pl/lib/libmathlib.so: $(math-lib-objs:%.o=%.os)
	$(CC) $(CFLAGS_PL) $(LDFLAGS) -shared -o $@ $^

build/pl/lib/libmathlib.a: $(math-lib-objs)
	rm -f $@
	$(AR) rc $@ $^
	$(RANLIB) $@

$(math-host-tools): HOST_LDLIBS += -lm -lmpfr -lmpc
$(math-tools): LDLIBS += $(math-ldlibs) -lm

# Some targets to build pl/math/test from math/test sources
build/pl/math/test/%.o: $(srcdir)/math/test/%.S
	$(CC) $(CFLAGS_PL) -c -o $@ $<

build/pl/math/test/%.o: $(srcdir)/math/test/%.c
	$(CC) $(CFLAGS_PL) -c -o $@ $<

build/pl/math/test/%.os: $(srcdir)/math/test/%.S
	$(CC) $(CFLAGS_PL) -c -o $@ $<

build/pl/math/test/%.os: $(srcdir)/math/test/%.c
	$(CC) $(CFLAGS_PL) -c -o $@ $<

# Some targets to build pl/ sources using appropriate flags
build/pl/%.o: $(srcdir)/pl/%.S
	$(CC) $(CFLAGS_PL) -c -o $@ $<

build/pl/%.o: $(srcdir)/pl/%.c
	$(CC) $(CFLAGS_PL) -c -o $@ $<

build/pl/%.os: $(srcdir)/pl/%.S
	$(CC) $(CFLAGS_PL) -c -o $@ $<

build/pl/%.os: $(srcdir)/pl/%.c
	$(CC) $(CFLAGS_PL) -c -o $@ $<

build/pl/bin/rtest: $(math-host-objs)
	$(HOST_CC) $(HOST_CFLAGS) $(HOST_LDFLAGS) -o $@ $^ $(HOST_LDLIBS)

build/pl/bin/mathtest: $(B)/test/mathtest.o build/pl/lib/libmathlib.a
	$(CC) $(CFLAGS_PL) $(LDFLAGS) -static -o $@ $^ $(LDLIBS)

build/pl/bin/mathbench: $(B)/test/mathbench.o build/pl/lib/libmathlib.a
	$(CC) $(CFLAGS_PL) $(LDFLAGS) -static -o $@ $^ $(LDLIBS)

# This is not ideal, but allows custom symbols in mathbench to get resolved.
build/pl/bin/mathbench_libc: $(B)/test/mathbench.o build/pl/lib/libmathlib.a
	$(CC) $(CFLAGS_PL) $(LDFLAGS) -static -o $@ $< $(LDLIBS) -lc build/pl/lib/libmathlib.a -lm

build/pl/bin/ulp: $(B)/test/ulp.o build/pl/lib/libmathlib.a
	$(CC) $(CFLAGS_PL) $(LDFLAGS) -static -o $@ $^ $(LDLIBS)

build/pl/include/%.h: $(PLM)/include/%.h
	cp $< $@

build/pl/include/test/%.h: $(PLM)/test/%.h
	cp $< $@

build/pl/bin/%.sh: $(PLM)/test/%.sh
	cp $< $@

pl-math-tests := $(wildcard $(PLM)/test/testcases/directed/*.tst)
pl-math-rtests := $(wildcard $(PLM)/test/testcases/random/*.tst)

check-pl/math-test: $(math-tools)
	cat $(pl-math-tests) | $(EMULATOR) build/pl/bin/mathtest $(math-testflags)

check-pl/math-rtest: $(math-host-tools) $(math-tools)
	cat $(pl-math-rtests) | build/pl/bin/rtest | $(EMULATOR) build/pl/bin/mathtest $(math-testflags)

check-pl/math-ulp: $(math-tools)
	WANT_ERRNO=$(WANT_ERRNO) WANT_SVE_MATH=$(WANT_SVE_MATH) ULPFLAGS="$(math-ulpflags)" build/pl/bin/runulp.sh $(EMULATOR)

check-pl/math: check-pl/math-test check-pl/math-rtest check-pl/math-ulp

$(DESTDIR)$(libdir)/pl/%.so: build/pl/lib/%.so
	$(INSTALL) -D $< $@

$(DESTDIR)$(libdir)/pl/%: build/pl/lib/%
	$(INSTALL) -m 644 -D $< $@

$(DESTDIR)$(includedir)/pl/%: build/pl/include/%
	$(INSTALL) -m 644 -D $< $@

install-pl/math: \
 $(math-libs:build/pl/lib/%=$(DESTDIR)$(libdir)/pl/%) \
 $(math-includes:build/pl/include/%=$(DESTDIR)$(includedir)/pl/%)

clean-pl/math:
	rm -f $(pl/math-files)

.PHONY: all-pl/math check-pl/math-test check-pl/math-rtest check-pl/math-ulp check-pl/math install-pl/math clean-pl/math
