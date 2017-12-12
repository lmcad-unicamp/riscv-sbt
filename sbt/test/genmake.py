#!/usr/bin/env python3

from auto.config import *
from auto.genmake import *


PROLOGUE = """\
include $(TOPDIR)/make/config.mk

SRCDIR      := $(shell pwd)
DSTDIR      := {0}

all: elf tests

clean:
	rm -rf $(DSTDIR)

### elf ###

CXX      = g++
CXXFLAGS = -m32 -Wall -Werror -g -std=c++11 -pedantic
LDFLAGS  = -m32

$(DSTDIR):
	mkdir -p $@

$(DSTDIR)/elf.o: elf.cpp elf.hpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

$(DSTDIR)/elf: $(DSTDIR) $(DSTDIR)/elf.o
	$(CXX) $(LDFLAGS) -o $@ $(DSTDIR)/elf.o

.PHONY: elf
elf: $(DSTDIR)/elf

### x86-syscall-test ###

.PHONY: x86-syscall-test
x86-syscall-test: $(DSTDIR)/x86-syscall-test

$(DSTDIR)/x86-syscall-test: $(SRCDIR)/x86-syscall-test.s
	$(BUILD_PY) --arch x86 --srcdir $(SRCDIR) --dstdir $(DSTDIR) \
		x86-syscall-test.s -o x86-syscall-test

.PHONY: x86-syscall-test-run
x86-syscall-test-run:
	$(RUN_PY) --arch x86 --dir $(DSTDIR) x86-syscall-test

""".format(mpath(DIR.build, "sbt", DIR.build_type_dir, "test"))


if __name__ == "__main__":
    narchs = [RV32, X86]
    xarchs = [(RV32, X86)]
    srcdir = "$(SRCDIR)"
    dstdir = "$(DSTDIR)"

    txt = PROLOGUE

    # tests

    rflags = "-o {}.out --tee"
    mods = [
        Module("hello", "{}-hello.s", xflags="-C", bflags="-C", rflags=rflags),
        Module("argv", "argv.c", rflags="one two three " + rflags),
        Module("mm", "mm.c", bflags='--cflags="-DROWS=4"', rflags=rflags),
    ]

    for mod in mods:
        name = mod.name
        src = mod.src
        xflags = mod.xflags
        bflags = mod.bflags
        rflags = mod.rflags
        txt = txt + do_mod(narchs, xarchs, name, src,
                srcdir, dstdir, xflags, bflags, rflags)

    names = [mod.name for mod in mods]
    tests = [name + "-test" for name in names]

    txt = txt + """\
### tests targets ###

.PHONY: tests
tests: x86-syscall-test {names}

.PHONY: tests-run
tests-run: tests x86-syscall-test-run {tests}

""".format(**{"names": " ".join(names), "tests": " ".join(tests)})


    # utests

    txt = txt + "### RV32 Translator unit tests ###\n\n"

    mods = [
        "load-store",
        "alu-ops",
        "branch",
        "fence",
        "system",
        "m"
    ]

    narchs = [RV32]
    xflags = "--sbtobjs syscall counters"
    bflags = None
    for mod in mods:
        name = mod
        src = "rv32-" + name + ".s"
        txt = txt + do_mod(narchs, xarchs, name, src,
                srcdir, dstdir, xflags, bflags, rflags)


    utests = [mod + "-test" for mod in mods if mod != "system"]

    fmtdata = {
        "mods":     " ".join(mods),
        "utests":   " ".join(utests),
    }

    txt = txt + """\
.PHONY: utests
utests: {mods}

# NOTE removed system from utests to run (need MSR access and performance
# counters support (not always available in VMs))
utests-run: utests {utests}
	@echo "All unit tests passed!"

###

.PHONY: alltests
alltests: tests utests

.PHONY: alltests-run
alltests-run: alltests tests-run utests-run
""".format(**fmtdata)

    # write txt to Makefile
    with open("Makefile", "w") as f:
        f.write(txt)

