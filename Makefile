# Makefile - requires GNU make
#
# Copyright (c) 2018, Arm Limited.
# SPDX-License-Identifier: MIT

srcdir = .
prefix = /usr
bindir = $(prefix)/bin
libdir = $(prefix)/lib
includedir = $(prefix)/include

MATH_SRCS = $(wildcard $(srcdir)/math/*.[cS])
MATH_BASE = $(basename $(MATH_SRCS))
MATH_OBJS = $(MATH_BASE:$(srcdir)/%=build/%.o)
RTEST_SRCS = $(wildcard $(srcdir)/test/rtest/*.[cS])
RTEST_BASE = $(basename $(RTEST_SRCS))
RTEST_OBJS = $(RTEST_BASE:$(srcdir)/%=build/%.o)
ALL_OBJS = $(MATH_OBJS) \
	$(RTEST_OBJS) \
	build/test/mathtest.o \
	build/test/mathbench.o \
	build/test/ulp.o \

INCLUDES = $(wildcard $(srcdir)/math/include/*.h)
ALL_INCLUDES = $(INCLUDES:$(srcdir)/math/%=build/%)

ALL_LIBS = \
	build/lib/libmathlib.so \
	build/lib/libmathlib.a \

ALL_TOOLS = \
	build/bin/mathtest \
	build/bin/mathbench \
	build/bin/mathbench_libc \
	build/bin/runulp.sh \
	build/bin/ulp \

HOST_TOOLS = \
	build/bin/rtest \

TESTS = $(wildcard $(srcdir)/test/testcases/directed/*.tst)
RTESTS = $(wildcard $(srcdir)/test/testcases/random/*.tst)

# Configure these in config.mk, do not make changes in this file.
HOST_CC = cc
HOST_CFLAGS = -std=c99 -O2
HOST_LDFLAGS =
HOST_LDLIBS = -lm -lmpfr -lmpc
EMULATOR =
CFLAGS = -std=c99 -O2
LDFLAGS =
LDLIBS = -lm
CPPFLAGS =
AR = $(CROSS_COMPILE)ar
RANLIB = $(CROSS_COMPILE)ranlib
INSTALL = install

CFLAGS_ALL = -I$(srcdir)/math/include $(CPPFLAGS) $(CFLAGS)
LDFLAGS_ALL = $(LDFLAGS)

-include config.mk

all: $(ALL_LIBS) $(ALL_TOOLS) $(ALL_INCLUDES)

DIRS = $(dir $(ALL_LIBS) $(ALL_TOOLS) $(ALL_OBJS) $(ALL_INCLUDES))
ALL_DIRS = $(sort $(DIRS:%/=%))

$(ALL_LIBS) $(ALL_TOOLS) $(ALL_OBJS) $(ALL_OBJS:%.o=%.os) $(ALL_INCLUDES): | $(ALL_DIRS)

$(ALL_DIRS):
	mkdir -p $@

$(ALL_OBJS:%.o=%.os): CFLAGS_ALL += -fPIC

$(RTEST_OBJS): CC = $(HOST_CC)
$(RTEST_OBJS): CFLAGS_ALL = $(HOST_CFLAGS)

build/test/mathtest.o: CFLAGS_ALL += -fmath-errno

build/test/ulp.o: $(srcdir)/test/ulp.h

build/%.o: $(srcdir)/%.S
	$(CC) $(CFLAGS_ALL) -c -o $@ $<

build/%.o: $(srcdir)/%.c
	$(CC) $(CFLAGS_ALL) -c -o $@ $<

build/%.os: $(srcdir)/%.S
	$(CC) $(CFLAGS_ALL) -c -o $@ $<

build/%.os: $(srcdir)/%.c
	$(CC) $(CFLAGS_ALL) -c -o $@ $<

build/lib/libmathlib.so: $(MATH_OBJS:%.o=%.os)
	$(CC) $(CFLAGS_ALL) $(LDFLAGS_ALL) -shared -o $@ $^

build/lib/libmathlib.a: $(MATH_OBJS)
	rm -f $@
	$(AR) rc $@ $^
	$(RANLIB) $@

build/bin/rtest: $(RTEST_OBJS)
	$(HOST_CC) $(HOST_CFLAGS) $(HOST_LDFLAGS) -o $@ $^ $(HOST_LDLIBS)

build/bin/mathtest: build/test/mathtest.o build/lib/libmathlib.a
	$(CC) $(CFLAGS_ALL) $(LDFLAGS_ALL) -static -o $@ $^ $(LDLIBS)

build/bin/mathbench: build/test/mathbench.o build/lib/libmathlib.a
	$(CC) $(CFLAGS_ALL) $(LDFLAGS_ALL) -static -o $@ $^ $(LDLIBS)

build/bin/mathbench_libc: build/test/mathbench.o
	$(CC) $(CFLAGS_ALL) $(LDFLAGS_ALL) -static -o $@ $^ $(LDLIBS)

build/bin/ulp: build/test/ulp.o build/lib/libmathlib.a
	$(CC) $(CFLAGS_ALL) $(LDFLAGS_ALL) -static -o $@ $^ $(LDLIBS)

build/include/%.h: $(srcdir)/math/include/%.h
	cp $< $@

build/bin/%.sh: $(srcdir)/test/%.sh
	cp $< $@

clean:
	rm -rf build

distclean: clean
	rm -f config.mk

$(DESTDIR)$(bindir)/%: build/bin/%
	$(INSTALL) -D $< $@

$(DESTDIR)$(libdir)/%.so: build/lib/%.so
	$(INSTALL) -D $< $@

$(DESTDIR)$(libdir)/%: build/lib/%
	$(INSTALL) -m 644 -D $< $@

$(DESTDIR)$(includedir)/%: build/include/%
	$(INSTALL) -m 644 -D $< $@

install-tools: $(ALL_TOOLS:build/bin/%=$(DESTDIR)$(bindir)/%)

install-libs: $(ALL_LIBS:build/lib/%=$(DESTDIR)$(libdir)/%)

install-headers: $(ALL_INCLUDES:build/include/%=$(DESTDIR)$(includedir)/%)

install: install-libs install-headers

check: $(ALL_TOOLS)
	cat $(TESTS) | $(EMULATOR) build/bin/mathtest

rcheck: $(HOST_TOOLS) $(ALL_TOOLS)
	cat $(RTESTS) | build/bin/rtest | $(EMULATOR) build/bin/mathtest

ucheck: $(ALL_TOOLS)
	build/bin/runulp.sh $(EMULATOR)

check-all: check rcheck ucheck

.PHONY: all clean distclean install install-tools install-libs install-headers check rcheck ucheck check-all
