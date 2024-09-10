# Makefile fragment - requires GNU make
#
# Copyright (c) 2019-2024, Arm Limited.
# SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception

ifeq ($(OS),Darwin)
  ifeq ($(WANT_SIMD_EXCEPT),1)
    $(error WANT_SIMD_EXCEPT is not supported on Darwin)
  endif
  ifeq ($(WANT_SVE_MATH),1)
    $(error WANT_SVE_MATH is not supported on Darwin)
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
B := build/pl/math

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
	build/pl/bin/ulp \

math-host-tools := \
	build/pl/bin/rtest \

pl-lib-objs := $(patsubst $(PLM)/%,$(B)/%.o,$(basename $(pl-lib-srcs)))
math-test-objs := $(patsubst $(AOR)/%,$(B)/%.o,$(basename $(math-test-srcs)))
math-host-objs := $(patsubst $(AOR)/%,$(B)/%.o,$(basename $(math-test-host-srcs)))
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
$(B)/test/mathtest.o: CFLAGS_PL += -fmath-errno
$(math-host-objs): CC = $(HOST_CC)
$(math-host-objs): CFLAGS_PL = $(HOST_CFLAGS)

$(B)/sv_%: CFLAGS_PL += $(math-sve-cflags)

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

$(B)/test/ulp.o: $(AOR)/test/ulp.h build/pl/include/test/ulp_funcs_gen.h build/pl/include/test/ulp_wrappers_gen.h
$(B)/test/ulp.o: CFLAGS_PL += -I build/pl/include/test

$(B)/test/mathbench.o: build/pl/include/test/mathbench_funcs_gen.h
$(B)/test/mathbench.o: CFLAGS_PL += -I build/pl/include/test

build/pl/lib/libmathlib.so: $(pl-lib-objs:%.o=%.os)
	$(CC) $(CFLAGS_PL) $(LDFLAGS) -shared -o $@ $^

build/pl/lib/libmathlib.a: $(pl-lib-objs)
	rm -f $@
	$(AR) rc $@ $^
	$(RANLIB) $@

$(math-host-tools): HOST_LDLIBS += -lm -lmpfr -lmpc
$(math-tools): LDLIBS += $(math-ldlibs) -lm
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

build/pl/bin/mathtest: $(B)/test/mathtest.o build/pl/lib/libmathlib.a
	$(CC) $(CFLAGS_PL) $(LDFLAGS) -o $@ $^ $(LDLIBS)

build/pl/bin/mathbench: $(B)/test/mathbench.o build/pl/lib/libmathlib.a
	$(CC) $(CFLAGS_PL) $(LDFLAGS) -o $@ $^ $(LDLIBS)

# This is not ideal, but allows custom symbols in mathbench to get resolved.
build/pl/bin/mathbench_libc: $(B)/test/mathbench.o build/pl/lib/libmathlib.a
	$(CC) $(CFLAGS_PL) $(LDFLAGS) -o $@ $< $(LDLIBS) -lc build/pl/lib/libmathlib.a -lm

build/pl/bin/ulp: $(B)/test/ulp.o build/pl/lib/libmathlib.a
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

ulp-input-dir=$(B)/test/inputs
$(ulp-input-dir):
	mkdir -p $@

math-lib-lims = $(patsubst $(PLM)/%,$(ulp-input-dir)/%.ulp,$(basename $(pl-lib-srcs)))
math-lib-fenvs = $(patsubst $(PLM)/%,$(ulp-input-dir)/%.fenv,$(basename $(pl-lib-srcs)))
math-lib-itvs = $(patsubst $(PLM)/%,$(ulp-input-dir)/%.itv,$(basename $(pl-lib-srcs)))

ulp-inputs = $(math-lib-lims) $(math-lib-fenvs) $(math-lib-itvs)

$(ulp-inputs): CFLAGS_PL += -I$(PLM) -I$(PLM)/include $(math-cflags)

$(ulp-input-dir)/%.ulp: $(PLM)/%.c | $(ulp-input-dir)
	$(CC) -I$(PLM)/test $(CFLAGS_PL) $< -o - -E | { grep -o "PL_TEST_ULP [^ ]* [^ ]*" || true; } > $@

$(ulp-input-dir)/%.fenv: $(PLM)/%.c | $(ulp-input-dir)
	$(CC) -I$(PLM)/test $(CFLAGS_PL) $< -o - -E | { grep -o "PL_TEST_EXPECT_FENV_ENABLED [^ ]*" || true; } > $@

$(ulp-input-dir)/%.itv: $(PLM)/%.c | $(ulp-input-dir)
	$(CC) -I$(PLM)/test $(CFLAGS_PL) $< -o - -E | { grep "PL_TEST_INTERVAL " || true; } | sed "s/ PL_TEST_INTERVAL/\nPL_TEST_INTERVAL/g" > $@

ulp-lims := $(ulp-input-dir)/limits
$(ulp-lims): $(math-lib-lims)
	cat $^ | sed "s/PL_TEST_ULP //g;s/^ *//g" > $@

fenv-exps := $(ulp-input-dir)/fenv
$(fenv-exps): $(math-lib-fenvs)
	cat $^ | sed "s/PL_TEST_EXPECT_FENV_ENABLED //g;s/^ *//g" > $@

ulp-itvs := $(ulp-input-dir)/intervals
$(ulp-itvs): $(math-lib-itvs)
	cat $^ | sort -u | sed "s/PL_TEST_INTERVAL //g" > $@

check-pl/math-ulp: $(math-tools) $(ulp-lims) $(fenv-exps) $(ulp-itvs)
	WANT_SVE_MATH=$(WANT_SVE_MATH) \
	ULPFLAGS="$(math-ulpflags)" \
	LIMITS=../../../$(ulp-lims) \
	INTERVALS=../../../$(ulp-itvs) \
	FENV=../../../$(fenv-exps) \
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
