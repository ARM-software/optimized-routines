# Makefile fragment - requires GNU make
#
# Copyright (c) 2019-2024, Arm Limited.
# SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception

ifneq ($(OS),Linux)
  ifeq ($(WANT_SIMD_EXCEPT),1)
    $(error WANT_SIMD_EXCEPT is not supported outside Linux)
  endif
  ifeq ($(WANT_SVE_MATH),1)
    $(error WANT_SVE_MATH is not supported outside Linux)
  endif
  ifneq ($(USE_MPFR),1)
    $(warning WARNING: Double-precision ULP tests will not be usable without MPFR)
  endif
  ifeq ($(USE_GLIBC_ABI),1)
    $(error Can only generate special GLIBC symbols on Linux - please disable USE_GLIBC_ABI)
  endif
endif

PLM := $(srcdir)/pl/math
AOR := $(srcdir)/math
PLB := build/pl/math

pl-lib-srcs := $(wildcard $(PLM)/*.[cS])

ifeq ($(WANT_SVE_MATH), 0)
pl-lib-srcs := $(filter-out $(PLM)/sv_%, $(pl-lib-srcs))
endif

ifneq ($(OS),Linux)
# Disable all vector symbols if not on Linux
pl-lib-srcs := $(filter-out $(PLM)/sv_%, $(pl-lib-srcs))
pl-lib-srcs := $(filter-out $(PLM)/v_%, $(pl-lib-srcs))
endif

math-test-srcs := \
	$(AOR)/test/mathtest.c \
	$(AOR)/test/mathbench.c \
	$(AOR)/test/ulp.c \

math-test-host-srcs := $(wildcard $(AOR)/test/rtest/*.[cS])

pl-includes := $(patsubst $(PLM)/%,build/pl/%,$(wildcard $(PLM)/include/*.h))
pl-test-includes := $(patsubst $(PLM)/%,build/pl/include/%,$(wildcard $(PLM)/test/*.h))

pl-libs := \
	build/pl/lib/libmathlib.so \
	build/pl/lib/libmathlib.a \

math-tools := \
	build/pl/bin/mathtest \
	build/pl/bin/mathbench \
	build/pl/bin/mathbench_libc \
	build/pl/bin/runulp.sh \
	build/pl/bin/ulp

math-host-tools := \
	build/pl/bin/rtest \

pl-lib-objs := $(patsubst $(PLM)/%,$(PLB)/%.o,$(basename $(pl-lib-srcs)))
math-test-objs := $(patsubst $(AOR)/%,$(PLB)/%.o,$(basename $(math-test-srcs)))
math-host-objs := $(patsubst $(AOR)/%,$(PLB)/%.o,$(basename $(math-test-host-srcs)))
pl-target-objs := $(pl-lib-objs) $(math-test-objs)
pl-objs := $(pl-target-objs) $(pl-target-objs:%.o=%.os) $(math-host-objs)

pl/math-files := \
	$(pl-objs) \
	$(pl-libs) \
	$(math-tools) \
	$(math-host-tools) \
	$(pl-includes) \
	$(pl-test-includes) \

all-pl/math: $(pl-libs) $(math-tools) $(pl-includes) $(pl-test-includes)

$(pl-objs): $(pl-includes) $(pl-test-includes)
$(pl-objs): CFLAGS_PL += $(math-cflags)
$(PLB)/test/mathtest.o: CFLAGS_PL += -fmath-errno
$(math-host-objs): CC = $(HOST_CC)
$(math-host-objs): CFLAGS_PL = $(HOST_CFLAGS)

$(PLB)/sv_%: CFLAGS_PL += $(math-sve-cflags)

ulp-funcs-dir = build/pl/test/ulp-funcs/
ulp-wrappers-dir = build/pl/test/ulp-wrappers/
mathbench-funcs-dir = build/pl/test/mathbench-funcs/
plsig-dirs = $(ulp-funcs-dir) $(ulp-wrappers-dir) $(mathbench-funcs-dir)

$(plsig-dirs):
	mkdir -p $@

ulp-funcs = $(patsubst $(PLM)/%,$(ulp-funcs-dir)%,$(basename $(pl-lib-srcs)))
ulp-wrappers = $(patsubst $(PLM)/%,$(ulp-wrappers-dir)%,$(basename $(pl-lib-srcs)))
mathbench-funcs = $(patsubst $(PLM)/%,$(mathbench-funcs-dir)%,$(basename $(pl-lib-srcs)))
plsig-autogen-files = $(ulp-funcs) $(ulp-wrappers) $(mathbench-funcs)

$(plsig-autogen-files): CFLAGS_PL += -DWANT_TRIGPI_TESTS=$(WANT_TRIGPI_TESTS)

$(ulp-funcs): PLSIG_DIRECTIVE = EMIT_ULP_FUNCS
$(ulp-wrappers): PLSIG_DIRECTIVE = EMIT_ULP_WRAPPERS
$(mathbench-funcs): PLSIG_DIRECTIVE = EMIT_MATHBENCH_FUNCS

.SECONDEXPANSION:
$(plsig-autogen-files): %: $(PLM)/$$(notdir $$@).c | $$(dir $$@)
	$(CC) $< $(CFLAGS_PL) -D$(PLSIG_DIRECTIVE) -E -o - | { grep PL_SIG || true; } | cut -f 2- -d ' ' > $@

build/pl/include/test/ulp_funcs_gen.h: $(ulp-funcs)
	cat $^ | sort -u > $@

build/pl/include/test/ulp_wrappers_gen.h: $(ulp-wrappers)
	cat $^ > $@

build/pl/include/test/mathbench_funcs_gen.h: $(mathbench-funcs)
	cat $^ | sort -u > $@

$(PLB)/test/ulp.o: $(AOR)/test/ulp.h build/pl/include/test/ulp_funcs_gen.h build/pl/include/test/ulp_wrappers_gen.h
$(PLB)/test/ulp.o: CFLAGS_PL += -I build/pl/include/test

$(PLB)/test/mathbench.o: build/pl/include/test/mathbench_funcs_gen.h
$(PLB)/test/mathbench.o: CFLAGS_PL += -I build/pl/include/test

build/pl/lib/libmathlib.so: $(pl-lib-objs:%.o=%.os)
	$(CC) $(CFLAGS_PL) $(LDFLAGS) -shared -o $@ $^

build/pl/lib/libmathlib.a: $(pl-lib-objs)
	rm -f $@
	$(AR) rc $@ $^
	$(RANLIB) $@

$(math-host-tools): HOST_LDLIBS += $(libm-libs) $(mpfr-libs) $(mpc-libs)
$(math-tools): LDLIBS += $(math-ldlibs) $(libm-libs)

# math-sve-cflags should be empty if WANT_SVE_MATH is not enabled
$(math-tools): CFLAGS_PL += $(math-sve-cflags)
ifneq ($(OS),Darwin)
  $(math-tools): LDFLAGS += -static
endif

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

build/pl/bin/mathtest: $(PLB)/test/mathtest.o build/pl/lib/libmathlib.a
	$(CC) $(CFLAGS_PL) $(LDFLAGS) -o $@ $^ $(libm-libs)

build/pl/bin/mathbench: $(PLB)/test/mathbench.o build/pl/lib/libmathlib.a
	$(CC) $(CFLAGS_PL) $(LDFLAGS) -o $@ $^ $(libm-libs)

# This is not ideal, but allows custom symbols in mathbench to get resolved.
build/pl/bin/mathbench_libc: $(PLB)/test/mathbench.o build/pl/lib/libmathlib.a
	$(CC) $(CFLAGS_PL) $(LDFLAGS) -o $@ $< $(libm-libs) $(libc-libs) build/pl/lib/libmathlib.a $(libm-libs)

build/pl/bin/ulp: $(PLB)/test/ulp.o build/pl/lib/libmathlib.a
	$(CC) $(CFLAGS_PL) $(LDFLAGS) -o $@ $^ $(LDLIBS)

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

pl-ulp-input-dir=$(PLB)/test/inputs
$(pl-ulp-input-dir):
	mkdir -p $@

pl-math-lib-lims = $(patsubst $(PLM)/%,$(pl-ulp-input-dir)/%.ulp,$(basename $(pl-lib-srcs)))
pl-math-lib-fenvs = $(patsubst $(PLM)/%,$(pl-ulp-input-dir)/%.fenv,$(basename $(pl-lib-srcs)))
pl-math-lib-itvs = $(patsubst $(PLM)/%,$(pl-ulp-input-dir)/%.itv,$(basename $(pl-lib-srcs)))

pl-ulp-inputs = $(pl-math-lib-lims) $(pl-math-lib-fenvs) $(pl-math-lib-itvs)
$(pl-ulp-inputs): CFLAGS_PL += -I$(PLM) -I$(PLM)/include $(math-cflags)

$(pl-ulp-input-dir)/%.ulp: $(PLM)/%.c | $(pl-ulp-input-dir)
	$(CC) -I$(PLM)/test $(CFLAGS_PL) $< -o - -E | { grep -o "PL_TEST_ULP [^ ]* [^ ]*" || true; } > $@

$(pl-ulp-input-dir)/%.fenv: $(PLM)/%.c | $(pl-ulp-input-dir)
	$(CC) -I$(PLM)/test $(CFLAGS_PL) $< -o - -E | { grep -o "PL_TEST_DISABLE_FENV [^ ]*" || true; } > $@

$(pl-ulp-input-dir)/%.itv: $(PLM)/%.c | $(pl-ulp-input-dir)
	$(CC) -I$(PLM)/test $(CFLAGS_PL) $< -o - -E | { grep "PL_TEST_INTERVAL " || true; } | sed "s/ PL_TEST_INTERVAL/\nPL_TEST_INTERVAL/g" > $@

pl-ulp-lims := $(pl-ulp-input-dir)/limits
$(pl-ulp-lims): $(pl-math-lib-lims)
	cat $^ | sed "s/PL_TEST_ULP //g;s/^ *//g" > $@

pl-fenv-exps := $(pl-ulp-input-dir)/fenv
$(pl-fenv-exps): $(pl-math-lib-fenvs)
	cat $^ | sed "s/PL_TEST_DISABLE_FENV //g;s/^ *//g" > $@

pl-ulp-itvs := $(pl-ulp-input-dir)/intervals
$(pl-ulp-itvs): $(pl-math-lib-itvs)
	cat $^ | sort -u | sed "s/PL_TEST_INTERVAL //g" > $@

check-pl/math-ulp: $(math-tools) $(pl-ulp-lims) $(pl-fenv-exps) $(pl-ulp-itvs)
	WANT_SVE_MATH=$(WANT_SVE_MATH) \
	ULPFLAGS="$(math-ulpflags)" \
	LIMITS=../../../$(pl-ulp-lims) \
	INTERVALS=../../../$(pl-ulp-itvs) \
	FENV=../../../$(pl-fenv-exps) \
	FUNC=$(func) \
	PRED=$(pred) \
	USE_MPFR=$(USE_MPFR) \
	build/pl/bin/runulp.sh $(EMULATOR)

check-pl/math: check-pl/math-test check-pl/math-rtest check-pl/math-ulp

$(DESTDIR)$(libdir)/pl/%.so: build/pl/lib/%.so
	$(INSTALL) -D $< $@

$(DESTDIR)$(libdir)/pl/%: build/pl/lib/%
	$(INSTALL) -m 644 -D $< $@

$(DESTDIR)$(includedir)/pl/%: build/pl/include/%
	$(INSTALL) -m 644 -D $< $@

install-pl/math: \
 $(pl-libs:build/pl/lib/%=$(DESTDIR)$(libdir)/pl/%) \
 $(pl-includes:build/pl/include/%=$(DESTDIR)$(includedir)/pl/%)

clean-pl/math:
	rm -f $(pl/math-files)

.PHONY: all-pl/math check-pl/math-test check-pl/math-rtest check-pl/math-ulp check-pl/math install-pl/math clean-pl/math
