#!/usr/bin/env python3

from config import *

import argparse

def xarch_brk(xarch):
    dash = xarch.index("-")
    farch = xarch[0 : dash]
    narch = xarch[dash + 1 :]
    return farch, narch


OUT = """\
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


### main ###

def bldnrun(arch, srcdir, dstdir, _in, out, bflags=None, rflags=None):
    global OUT

    fmtdata = {
        "arch":     arch,
        "srcdir":   srcdir,
        "dstdir":   dstdir,
        "in":       _in,
        "out":      out,
        "bflags":   " " + bflags if bflags else "",
        "rflags":   " " + rflags if rflags else "",
    }

    OUT = OUT + """\
.PHONY: {out}
{out}: {dstdir}/{out}

{dstdir}/{out} {dstdir}/{out}.o: {srcdir}/{in}
	$(BUILD_PY) --arch {arch} --srcdir {srcdir} --dstdir {dstdir} {in} -o {out}{bflags}

.PHONY: {out}-run
{out}-run: {out}
	$(RUN_PY) --arch {arch} --dir {dstdir} {out}{rflags}

""".format(**fmtdata)


def xlatenrun(arch, srcdir, dstdir, _in, out, mode, xflags=None, rflags=None):
    global OUT

    flags = '--flags " -regs={}"'.format(mode)
    fmtdata = {
        "arch":     arch,
        "srcdir":   srcdir,
        "dstdir":   dstdir,
        "in":       _in,
        "out":      out + "-" + mode,
        "mode":     mode,
        "xflags":   " " + xflags if xflags else "",
        "rflags":   " " + rflags if rflags else "",
        "flags":    flags
    }

    OUT = OUT + """\
.PHONY: {out}
{out}: {dstdir}/{out}

{dstdir}/{out}: {dstdir}/{in}
	$(XLATE_PY) --arch {arch} --srcdir {srcdir} --dstdir {dstdir} {in} -o {out}{xflags} {flags}

.PHONY: {out}-run
{out}-run: {out}
	$(RUN_PY) --arch {arch} --dir {dstdir} {out}{rflags}

""".format(**fmtdata)


def test(xarchs, dir, name, ntest=False):
    global OUT

    diffs = []
    def diff(bin1, bin2):
        diff = "\tdiff {0}/{1}.out {0}/{2}.out".format(dir, bin1, bin2)
        diffs.append(diff)

    farchs = []
    narchs = []
    for xarch in xarchs:
        farch, narch = xarch_brk(xarch)
        farchs.append(farch)
        narchs.append(narch)

        fbin = farch + "-" + name

        for mode in SBT.modes:
            xbin = xarch + "-" + name + "-" + mode
            diff(fbin, xbin)
            if ntest:
                nbin = narch + "-" + name
                diff(nbin, xbin)

    runs = [arch + "-" + name + "-run"
            for arch in farchs + (narchs if ntest else [])]

    runs.extend([arch + "-" + name + "-" + mode + "-run"
            for arch in xarchs
            for mode in SBT.modes])

    fmtdata = {
        "name":     name,
        "runs":     " ".join(runs),
        "diffs":    "\n".join(diffs)
    }

    OUT = OUT + """\
.PHONY: {name}-test
{name}-test: {runs}
{diffs}

""".format(**fmtdata)


def aliases(narchs, xarchs, mod):
    global OUT

    nmods = [arch + "-" + mod for arch in narchs]
    xmods = [arch + "-" + mod + "-" + mode
            for arch in xarchs
            for mode in SBT.modes]

    runs = [m + "-run" for m in nmods + xmods]

    fmtdata = {
        "mod":      mod,
        "nmods":    " ".join(nmods),
        "xmods":    " ".join(xmods),
        "runs":     " ".join(runs),
    }

    OUT = OUT + """\
.PHONY: {mod}
{mod}: {nmods} {xmods}

.PHONY: {mod}-run
{mod}-run: {runs}

""".format(**fmtdata)


class Module:
    def __init__(self, name, src, xflags=None, bflags=None, rflags=None):
        self.name = name
        self.src = src
        self.xflags = xflags
        self.bflags = bflags
        self.rflags = rflags


if __name__ == "__main__":
    srcdir = "$(SRCDIR)"
    dstdir = "$(DSTDIR)"
    narchs = ["rv32", "x86"]
    xarchs = ["rv32-x86"]

    srcname = lambda templ, arch: templ.format(arch)

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

        # module comment
        OUT = OUT + "### {} ###\n\n".format(name)

        # native builds
        for arch in narchs:
            _in = srcname(src, arch)
            out = arch + "-" + name
            bldnrun(arch, srcdir, dstdir, _in, out, bflags, rflags.format(out))

        # translations
        for xarch in xarchs:
            (farch, narch) = xarch_brk(xarch)
            fmod  = farch + "-" + name
            nmod  = farch + "-" + narch + "-" + name

            for mode in SBT.modes:
                xlatenrun(narch, srcdir, dstdir, fmod + ".o", nmod,
                        mode, xflags, rflags.format(nmod + "-" + mode))

        # tests
        test(xarchs, dstdir, name, ntest=True)

        # aliases
        aliases(narchs, xarchs, name)

    names = [mod.name for mod in mods]
    tests = [name + "-test" for name in names]

    OUT = OUT + """\
### tests targets ###

.PHONY: tests
tests: x86-syscall-test {names}

.PHONY: tests-run
tests-run: tests x86-syscall-test-run {tests}

""".format(**{"names": " ".join(names), "tests": " ".join(tests)})


    # write OUT to Makefile
    with open("Makefile", "w") as f:
        f.write(OUT)


"""
### RV32 Translator unit tests ###

UTNARCHS := rv32
UTESTS   := load-store alu-ops branch fence system m

$(foreach test,$(UTESTS),$(eval \
$(call UTEST,$(UTNARCHS),$(test),$(SRCDIR),$(DSTDIR),\
--asm,--counters,--save-output,,add-arch-prefix)))

.PHONY: utests
utests: $(UTESTS)

# NOTE removed system from utests to run (need MSR access and performance
# counters support (not always available in VMs))
utests-run: utests $(foreach test,$(filter-out system,$(UTESTS)),$(test)-test)
	@echo "All unit tests passed!"

###

.PHONY: alltests
alltests: tests utests

.PHONY: alltests-run
alltests-run: alltests tests-run utests-run
"""
