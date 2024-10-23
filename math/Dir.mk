# Makefile fragment - requires GNU make
#
# Copyright (c) 2019-2024, Arm Limited.
# SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception

.SECONDEXPANSION:

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

ulp-funcs-dir = build/test/ulp-funcs/
ulp-wrappers-dir = build/test/ulp-wrappers/
mathbench-funcs-dir = build/test/mathbench-funcs/
test-sig-dirs = $(ulp-funcs-dir) $(ulp-wrappers-dir) $(mathbench-funcs-dir)
$(test-sig-dirs) $(addsuffix /$(ARCH),$(test-sig-dirs)):
	mkdir -p $@

ulp-funcs = $(patsubst $(S)/%,$(ulp-funcs-dir)/%,$(basename $(math-lib-srcs)))
ulp-wrappers = $(patsubst $(S)/%,$(ulp-wrappers-dir)/%,$(basename $(math-lib-srcs)))
mathbench-funcs = $(patsubst $(S)/%,$(mathbench-funcs-dir)/%,$(basename $(math-lib-srcs)))

define emit_sig
$1/%: $(S)/%.c | $$$$(@D)
	$(CC) $$< $(math-cflags) -I$(S)/include -D$2 -E -o - | { grep TEST_SIG || true; } | cut -f 2- -d ' ' > $$@
endef

$(eval $(call emit_sig,$(ulp-funcs-dir),EMIT_ULP_FUNCS))
$(eval $(call emit_sig,$(ulp-wrappers-dir),EMIT_ULP_WRAPPERS))
$(eval $(call emit_sig,$(mathbench-funcs-dir),EMIT_MATHBENCH_FUNCS))

ulp-funcs-gen = build/include/test/ulp_funcs_gen.h
ulp-wrappers-gen = build/include/test/ulp_wrappers_gen.h
mathbench-funcs-gen = build/include/test/mathbench_funcs_gen.h
math-tools-autogen-headers = $(ulp-funcs-gen) $(ulp-wrappers-gen) $(mathbench-funcs-gen)

$(ulp-funcs-gen): $(ulp-funcs)
$(ulp-wrappers-gen): $(ulp-wrappers)
$(mathbench-funcs-gen): $(mathbench-funcs)

$(math-tools-autogen-headers): | $$(@D)
	cat $^ | sort -u > $@

$(B)/test/mathbench.o: $(mathbench-funcs-gen)
$(B)/test/ulp.o: $(S)/test/ulp.h $(ulp-funcs-gen) $(ulp-wrappers-gen)

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

ulp-input-dir = $(B)/test/inputs
$(ulp-input-dir) $(ulp-input-dir)/$(ARCH):
	mkdir -p $@

math-lib-lims = $(patsubst $(S)/%.c,$(ulp-input-dir)/%.ulp,$(math-lib-srcs))
math-lib-lims-nn = $(patsubst $(S)/%.c,$(ulp-input-dir)/%.ulp_nn,$(math-lib-srcs))
math-lib-fenvs = $(patsubst $(S)/%.c,$(ulp-input-dir)/%.fenv,$(math-lib-srcs))
math-lib-itvs = $(patsubst $(S)/%.c,$(ulp-input-dir)/%.itv,$(math-lib-srcs))

ulp-inputs = $(math-lib-lims) $(math-lib-lims-nn) $(math-lib-fenvs) $(math-lib-itvs)
$(ulp-inputs): CFLAGS = -I$(S)/test -I$(S)/include $(math-cflags)

$(ulp-input-dir)/%.ulp: $(S)/%.c | $$(@D)
	$(CC) $(CFLAGS) $< -o - -E | { grep "TEST_ULP " || true; } > $@

$(ulp-input-dir)/%.ulp_nn: $(S)/%.c | $$(@D)
	$(CC) $(CFLAGS) $< -o - -E | { grep "TEST_ULP_NONNEAREST " || true; } > $@

$(ulp-input-dir)/%.fenv: $(S)/%.c | $$(@D)
	$(CC) $(CFLAGS) $< -o - -E | { grep "TEST_DISABLE_FENV " || true; } > $@

$(ulp-input-dir)/%.itv: $(S)/%.c | $$(@D)
	$(CC) $(CFLAGS) $< -o - -E | { grep "TEST_INTERVAL " || true; } | sed "s/ TEST_INTERVAL/\nTEST_INTERVAL/g" > $@

ulp-lims = $(ulp-input-dir)/limits
$(ulp-lims): $(math-lib-lims)

ulp-lims-nn = $(ulp-input-dir)/limits_nn
$(ulp-lims-nn): $(math-lib-lims-nn)

fenv-exps := $(ulp-input-dir)/fenv
$(fenv-exps): $(math-lib-fenvs)

generic-itvs = $(ulp-input-dir)/itvs
$(generic-itvs): $(filter-out $(ulp-input-dir)/$(ARCH)/%,$(math-lib-itvs))

arch-itvs = $(ulp-input-dir)/$(ARCH)/itvs
$(arch-itvs): $(filter $(ulp-input-dir)/$(ARCH)/%,$(math-lib-itvs))

# Remove first word, which will be TEST directive
$(ulp-lims) $(ulp-lims-nn) $(fenv-exps) $(arch-itvs) $(generic-itvs): | $$(@D)
	sed "s/TEST_[^ ]* //g" $^ | sort -u > $@

check-math-ulp: $(ulp-lims) $(ulp-lims-nn)
check-math-ulp: $(fenv-exps)
check-math-ulp: $(generic-itvs) $(arch-itvs)
check-math-ulp: $(math-tools)
	ULPFLAGS="$(math-ulpflags)" \
	LIMITS=../../$(ulp-lims) \
	ARCH_ITVS=../../$(arch-itvs) \
	GEN_ITVS=../../$(generic-itvs) \
	DISABLE_FENV=../../$(fenv-exps) \
	FUNC=$(func) \
	build/bin/runulp.sh $(EMULATOR)

check-math: check-math-test check-math-rtest check-math-ulp

install-math: \
 $(math-libs:build/lib/%=$(DESTDIR)$(libdir)/%) \
 $(math-includes:build/include/%=$(DESTDIR)$(includedir)/%)

clean-math:
	rm -f $(math-files)

.PHONY: all-math check-math-test check-math-rtest check-math-ulp check-math install-math clean-math
