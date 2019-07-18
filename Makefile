# Makefile - requires GNU make
#
# Copyright (c) 2018-2019, Arm Limited.
# SPDX-License-Identifier: MIT

srcdir = .
prefix = /usr
bindir = $(prefix)/bin
libdir = $(prefix)/lib
includedir = $(prefix)/include

# Build targets
ALL_OBJS = $(math-objs)
ALL_INCLUDES = $(math-includes)
ALL_LIBS = $(math-libs)
ALL_TOOLS = $(math-tools)
HOST_TOOLS = $(math-host-tools)

# Configure these in config.mk, do not make changes in this file.
HOST_CC = cc
HOST_CFLAGS = -std=c99 -O2
HOST_LDFLAGS =
HOST_LDLIBS =
EMULATOR =
CFLAGS = -std=c99 -O2
LDFLAGS =
LDLIBS =
CPPFLAGS =
AR = $(CROSS_COMPILE)ar
RANLIB = $(CROSS_COMPILE)ranlib
INSTALL = install

CFLAGS_ALL = -Ibuild/include $(CPPFLAGS) $(CFLAGS)
LDFLAGS_ALL = $(LDFLAGS)

all:

-include config.mk

include math/Dir.mk

all: all-math

DIRS = $(dir $(ALL_LIBS) $(ALL_TOOLS) $(ALL_OBJS) $(ALL_INCLUDES))
ALL_DIRS = $(sort $(DIRS:%/=%))

$(ALL_LIBS) $(ALL_TOOLS) $(ALL_OBJS) $(ALL_OBJS:%.o=%.os) $(ALL_INCLUDES): | $(ALL_DIRS)

$(ALL_DIRS):
	mkdir -p $@

$(ALL_OBJS:%.o=%.os): CFLAGS_ALL += -fPIC

build/%.o: $(srcdir)/%.S
	$(CC) $(CFLAGS_ALL) -c -o $@ $<

build/%.o: $(srcdir)/%.c
	$(CC) $(CFLAGS_ALL) -c -o $@ $<

build/%.os: $(srcdir)/%.S
	$(CC) $(CFLAGS_ALL) -c -o $@ $<

build/%.os: $(srcdir)/%.c
	$(CC) $(CFLAGS_ALL) -c -o $@ $<

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

check: check-math

.PHONY: all clean distclean install install-tools install-libs install-headers check
