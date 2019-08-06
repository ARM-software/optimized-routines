# Makefile fragment - requires GNU make
#
# Copyright (c) 2019, Arm Limited.
# SPDX-License-Identifier: MIT

string-lib-srcs := $(wildcard $(srcdir)/string/*.[cS])
string-includes-src := $(wildcard $(srcdir)/string/include/*.h)
string-includes := $(string-includes-src:$(srcdir)/string/%=build/%)
string-test-srcs := $(wildcard $(srcdir)/string/test/*.c)

string-libs := \
	build/lib/libstringlib.so \
	build/lib/libstringlib.a \

string-tools := \
	build/bin/test/memcpy \
	build/bin/test/memmove \
	build/bin/test/memset \
	build/bin/test/memchr \
	build/bin/test/memcmp

string-lib-base := $(basename $(string-lib-srcs))
string-lib-objs := $(string-lib-base:$(srcdir)/%=build/%.o)
string-test-base := $(basename $(string-test-srcs))
string-test-objs := $(string-test-base:$(srcdir)/%=build/%.o)

string-objs := \
	$(string-lib-objs) \
	$(string-test-objs) \

all-string: $(string-libs) $(string-tools) $(string-includes)

$(string-objs) $(string-objs:%.o=%.os): $(string-includes)

build/lib/libstringlib.so: $(string-lib-objs:%.o=%.os)
	$(CC) $(CFLAGS_ALL) $(LDFLAGS_ALL) -shared -o $@ $^

build/lib/libstringlib.a: $(string-lib-objs)
	rm -f $@
	$(AR) rc $@ $^
	$(RANLIB) $@

build/bin/test/%: build/string/test/%.o build/lib/libstringlib.a
	$(CC) $(CFLAGS_ALL) $(LDFLAGS_ALL) -static -o $@ $^ $(LDLIBS)

build/include/%.h: $(srcdir)/string/include/%.h
	cp $< $@

build/bin/%.sh: $(srcdir)/string/test/%.sh
	cp $< $@

check-string: $(string-tools)
	$(EMULATOR) build/bin/test/memcpy
	$(EMULATOR) build/bin/test/memmove
	$(EMULATOR) build/bin/test/memset
	$(EMULATOR) build/bin/test/memchr
	$(EMULATOR) build/bin/test/memcmp

.PHONY: all-string check-string
