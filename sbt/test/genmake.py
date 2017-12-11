#!/usr/bin/env python3

from config import *

import argparse



def xarch_brk(xarch):
    dash = xarch.index("-")
    farch = xarch[0 : dash]
    narch = xarch[dash + 1 :]
    return farch, narch


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


### main ###

def bldnrun(arch, srcdir, dstdir, _in, out, bflags=None, rflags=None):
    fmtdata = {
        "arch":     arch,
        "srcdir":   srcdir,
        "dstdir":   dstdir,
        "in":       _in,
        "out":      out,
        "bflags":   " " + bflags if bflags else "",
        "rflags":   " " + rflags if rflags else "",
    }

    txt = """\
.PHONY: {out}
{out}: {dstdir}/{out}

{dstdir}/{out} {dstdir}/{out}.o: {srcdir}/{in}
	$(BUILD_PY) --arch {arch} --srcdir {srcdir} --dstdir {dstdir} {in} -o {out}{bflags}

.PHONY: {out}-run
{out}-run: {out}
	$(RUN_PY) --arch {arch} --dir {dstdir} {out}{rflags}

""".format(**fmtdata)

    return txt


def xlatenrun(arch, srcdir, dstdir, _in, out, mode, xflags=None, rflags=None):
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

    txt = """\
.PHONY: {out}
{out}: {dstdir}/{out}

{dstdir}/{out}: {dstdir}/{in}
	$(XLATE_PY) --arch {arch} --srcdir {srcdir} --dstdir {dstdir} {in} -o {out}{xflags} {flags}

.PHONY: {out}-run
{out}-run: {out}
	$(RUN_PY) --arch {arch} --dir {dstdir} {out}{rflags}

""".format(**fmtdata)

    return txt


def test(xarchs, dir, name, ntest=False):
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

    txt = """\
.PHONY: {name}-test
{name}-test: {runs}
{diffs}

""".format(**fmtdata)

    return txt


def aliases(narchs, xarchs, mod):
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

    return """\
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



def _do_mod(narchs, xarchs, name, src, srcdir, dstdir, xflags, bflags, rflags):
    srcname = lambda templ, arch: templ.format(arch)

    # module comment
    txt = "### {} ###\n\n".format(name)

    # native builds
    for arch in narchs:
        _in = srcname(src, arch)
        out = arch + "-" + name
        txt = txt + \
            bldnrun(arch, srcdir, dstdir, _in, out, bflags, rflags.format(out))

    # translations
    for xarch in xarchs:
        (farch, narch) = xarch_brk(xarch)
        fmod  = farch + "-" + name
        nmod  = farch + "-" + narch + "-" + name

        for mode in SBT.modes:
            txt = txt + xlatenrun(narch, srcdir, dstdir, fmod + ".o", nmod,
                    mode, xflags, rflags.format(nmod + "-" + mode))

    # tests
    txt = txt + test(xarchs, dstdir, name, ntest=len(narchs) > 1)

    # aliases
    txt = txt + aliases(narchs, xarchs, name)

    return txt


if __name__ == "__main__":
    narchs = ["rv32", "x86"]
    xarchs = ["rv32-x86"]
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
        txt = txt + _do_mod(narchs, xarchs, name, src,
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

    narchs = ["rv32"]
    xflags = "--sbtobjs syscall counters"
    bflags = None
    for mod in mods:
        name = mod
        src = "rv32-" + name + ".s"
        txt = txt + _do_mod(narchs, xarchs, name, src,
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

    # write OUT to Makefile
    with open("Makefile", "w") as f:
        f.write(txt)

