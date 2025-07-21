# Makefile fragment - requires GNU make
#
# Copyright (c) 2019-2025, Arm Limited.
# SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception

.SECONDEXPANSION:

fp-src-dir := $(srcdir)/fp
fp-build-dir := build/fp

fp-lib-srcs := \
	$(wildcard $(fp-src-dir)/$(FP_SUBDIR)/*.[cS]) \
	$(wildcard $(fp-src-dir)/common/*.[cS]) \

fp-libs := \
	build/lib/libfplib.a \

fp-lib-objs := $(patsubst $(fp-src-dir)/%,$(fp-build-dir)/%.o,$(basename $(fp-lib-srcs)))

# The full list of FP test programs
all-fp-testnames := test-f2d test-d2f \
	test-fmul test-faddsub test-fdiv test-fcmp \
	test-f2uiz test-f2iz test-f2ulz test-f2lz \
	test-i2f test-ui2f test-l2f test-ul2f \
	test-dmul test-daddsub test-ddiv test-dcmp \
	test-d2uiz test-d2iz test-d2ulz test-d2lz \
	test-i2d test-ui2d test-l2d test-ul2d

# Filter the list down to only the tests of functions present in this FP_SUBDIR
fp-tests-available := $(foreach obj,$(fp-lib-objs),$(patsubst %.o,test-%,$(notdir $(obj))))
ifneq ($(findstring fcmp_,$(fp-tests-available)),)
fp-tests-available += test-fcmp
endif
ifneq ($(findstring dcmp_,$(fp-tests-available)),)
fp-tests-available += test-dcmp
endif
ifneq ($(findstring l2f,$(fp-tests-available)),)
fp-tests-available += test-ul2f
endif
fp-testnames := $(filter $(all-fp-testnames), $(fp-tests-available))

fp-tests := $(patsubst %,$(fp-build-dir)/%,$(fp-testnames))
fp-test-objs := $(patsubst %,$(fp-build-dir)/test/%.o,$(fp-testnames))

fp-target-objs := $(fp-lib-objs) $(fp-test-objs)
fp-objs := $(fp-target-objs)

fp-aux :=

ifeq ($(FP_SUBDIR),at32)
fp-objs += $(fp-build-dir)/at32/ddiv-diagnostics.o
$(fp-build-dir)/at32/ddiv-diagnostics.o: $(fp-src-dir)/at32/ddiv.S
	$(CC) $(CFLAGS_ALL) -c -o $@ $^ -DDIAGNOSTICS

fp-aux += $(fp-build-dir)/ddiv-diagnostics
endif

fp-files := \
	$(fp-objs) \
	$(fp-libs) \
	$(fp-tests) \
	$(fp-aux) \

all-fp: $(fp-libs) $(fp-tests) $(fp-aux)

$(fp-objs): $(fp-includes) $(fp-test-includes)
$(fp-objs): CFLAGS_ALL += $(fp-cflags)
$(fp-objs): CFLAGS_ALL += -I$(fp-src-dir)/include
ifeq ($(FP_SUBDIR),at32)
$(fp-objs): CFLAGS_ALL += -Wa,-mimplicit-it=always
endif

build/lib/libfplib.a: $(fp-lib-objs)
	rm -f $@
	$(AR) rc $@ $^
	$(RANLIB) $@

$(fp-tests): $(fp-build-dir)/%: $(fp-build-dir)/test/%.o $(fp-libs)
	$(CC) $(CFLAGS_ALL) $(LDFLAGS) -o $@ $^ $(fp-libs)

ifeq ($(FP_SUBDIR),at32)

$(fp-build-dir)/ddiv-diagnostics: $(fp-src-dir)/aux/ddiv-diagnostics.c \
	$(fp-build-dir)/at32/ddiv-diagnostics.o $(fp-libs)
	$(CC) $(CFLAGS_ALL) $(LDFLAGS) -o $@ $^ $(fp-libs)

endif

clean-fp:
	rm -f $(fp-files)

.PHONY: all-fp clean-fp
