# Makefile - requires GNU make
#
# Copyright (c) 2018, Arm Limited.
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

srcdir = .
prefix = /usr
bindir = $(prefix)/bin
libdir = $(prefix)/lib
includedir = $(prefix)/include

HACK = $(srcdir)/math/s_sincosf.c

MATH_SRCS = $(filter-out $(HACK),$(wildcard $(srcdir)/math/*.[cS]))
MATH_BASE = $(basename $(MATH_SRCS))
MATH_OBJS = $(MATH_BASE:$(srcdir)/%=build/%.o)
RTEST_SRCS = $(wildcard $(srcdir)/test/rtest/*.[cS])
RTEST_BASE = $(basename $(RTEST_SRCS))
RTEST_OBJS = $(RTEST_BASE:$(srcdir)/%=build/%.o)
ALL_OBJS = $(MATH_OBJS) $(RTEST_OBJS) build/test/mathtest.o

INCLUDES = $(wildcard $(srcdir)/math/include/*.h)
ALL_INCLUDES = $(INCLUDES:$(srcdir)/math/%=build/%)

ALL_LIBS = \
	build/lib/libmathlib.so \
	build/lib/libmathlib.a \

ALL_TOOLS = \
	build/bin/runtest.sh \
	build/bin/rtest \
	build/bin/mathtest \

TESTS = $(wildcard $(srcdir)/test/testcases/*/*.tst)
ALL_TESTS = $(TESTS:$(srcdir)/test/testcases/%=build/bin/%)

# Configure these in config.mk, do not make changes in this file.
HOST_CC = cc
EMULATOR =
CFLAGS = -std=c99 -O2
LDFLAGS =
CPPFLAGS =
AR = $(CROSS_COMPILE)ar
RANLIB = $(CROSS_COMPILE)ranlib
INSTALL = install

CFLAGS_ALL = -I$(srcdir)/math/include $(CPPFLAGS) $(CFLAGS)
LDFLAGS_ALL = $(LDFLAGS)

-include config.mk

all: $(ALL_LIBS) $(ALL_TOOLS) $(ALL_INCLUDES)

DIRS = $(dir $(ALL_LIBS) $(ALL_TOOLS) $(ALL_OBJS) $(ALL_INCLUDES) $(ALL_TESTS))
ALL_DIRS = $(sort $(DIRS:%/=%))

$(ALL_LIBS) $(ALL_TOOLS) $(ALL_OBJS) $(ALL_OBJS:%.o=%.os) $(ALL_INCLUDES) $(ALL_TESTS): | $(ALL_DIRS)

$(ALL_DIRS):
	mkdir -p $@

$(ALL_OBJS:%.o=%.os): CFLAGS_ALL += -fPIC

$(RTEST_OBJS): CC = $(HOST_CC)

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
	$(HOST_CC) $(CFLAGS_ALL) $(LDFLAGS_ALL) -o $@ $^ -lm -lmpfr -lmpc

build/bin/mathtest: build/test/mathtest.o build/lib/libmathlib.a
	$(CC) $(CFLAGS_ALL) $(LDFLAGS_ALL) -static -o $@ $^ -lm

build/include/%.h: $(srcdir)/math/include/%.h
	cp $< $@

build/bin/runtest.sh: $(srcdir)/test/runtest.sh
	cp $< $@

build/bin/%.tst: $(srcdir)/test/testcases/%.tst
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

check: $(ALL_TOOLS) $(ALL_TESTS)
	build/bin/runtest.sh $(EMULATOR) ./mathtest

.PHONY: all clean distclean install install-tools install-libs install-headers check
