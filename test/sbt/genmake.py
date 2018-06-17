#!/usr/bin/env python3

from auto.config import DIR, GOPTS, RV32_LINUX, SBT, TOOLS, X86
from auto.genmake import ArchAndMode, GenMake, Run, Runs
from auto.utils import cat, path

class Module:
    def __init__(self,
            name, src,
            xarchs, narchs,
            srcdir, dstdir,
            xflags=None, bflags=None, rflags=None, sbtflags=[],
            dbg=True):
        self.name = name
        self.src = src
        self.xarchs = xarchs
        self.narchs = narchs
        self.srcdir = srcdir
        self.dstdir = dstdir
        self.bflags = cat(bflags, "--dbg" if dbg else '')
        self.xflags = cat(self.bflags, xflags)
        self.rflags = rflags
        self.gm = GenMake(
            self.narchs, self.xarchs,
            self.srcdir, self.dstdir, name,
            self.xflags, self.bflags, mflags=None, sbtflags=sbtflags)

        self.robj = Run(args=[], rflags=self.rflags)
        self.runs = Runs([self.robj], name)


    def _srcname(self, templ, arch):
        return templ.format(arch.prefix)


    def _bldnrun(self, am, ins, out):
        self.gm.bld(am.narch, ins, out)
        self.gm.run(self.name, self.robj, am)


    def _xlatenrun(self, am):
        name = self.name
        fmod = am.farch.out2objname(name)
        nmod = am.bin(name)

        self.gm.xlate(am, fmod, nmod)
        self.gm.run(name, self.robj, am)


    def gen(self):
        name = self.name
        src = self.src

        # native builds
        for arch in self.narchs:
            _in = self._srcname(src, arch)
            out = arch.add_prefix(name)
            am = ArchAndMode(None, arch)
            self._bldnrun(am, [_in], out)

        # translations
        for (farch, narch) in self.xarchs:
            for mode in SBT.modes:
                am = ArchAndMode(farch, narch, mode)
                self._xlatenrun(am)

        # tests
        self.gm.test(self.runs)

        # aliases
        self.gm.alias_build_all()
        self.gm.alias_run_all(self.runs)

        return self.gm.txt


class Tests():
    def __init__(self):
        self.narchs = [RV32_LINUX, X86]
        self.xarchs = [(RV32_LINUX, X86)]
        self.srcdir = path(DIR.top, "test/sbt")
        self.dstdir = path(DIR.build, "test/sbt")
        self.sbtdir = path(DIR.top, "sbt")
        self.bflags = "--cc=" + GOPTS.cc
        self.txt = ''


    def append(self, txt):
        self.txt = self.txt + txt


    def gen_prologue(self):
        self.append("""\
all: elf tests utests

### elf ###

CXX      = g++
CXXFLAGS = -m32 -Wall -Werror -g -std=c++11 -pedantic
LDFLAGS  = -m32

{dstdir}:
\tmkdir -p $@

{dstdir}/elf.o: elf.cpp elf.hpp
\t$(CXX) $(CXXFLAGS) -o $@ -c $<

{dstdir}/elf: {dstdir} {dstdir}/elf.o
\t$(CXX) $(LDFLAGS) -o $@ {dstdir}/elf.o

.PHONY: elf
elf: {dstdir}/elf

### x86-syscall-test ###

.PHONY: x86-syscall-test
x86-syscall-test: {dstdir}/x86-syscall-test

{dstdir}/x86-syscall-test: {srcdir}/x86-syscall-test.s
\t{build} --arch x86 --srcdir {srcdir} --dstdir {dstdir} x86-syscall-test.s -o x86-syscall-test

.PHONY: x86-syscall-test-run
x86-syscall-test-run:
\t{run} --arch x86 --dir {dstdir} x86-syscall-test

### x86-fp128 ###

.PHONY: x86-fp128
x86-fp128: {dstdir}/x86-fp128

{dstdir}/x86-fp128: {srcdir}/x86-fp128.c
\t{build} --arch x86 --srcdir {srcdir} --dstdir {dstdir} x86-fp128.c \
    -o x86-fp128 --cflags="-I{sbtdir}" --dbg

.PHONY: x86-fp128-run
x86-fp128-run:
\t{run} --arch x86 --dir {dstdir} x86-fp128

""".format(**{
        "build":    TOOLS.build,
        "run":      TOOLS.run,
        "srcdir":   self.srcdir,
        "dstdir":   self.dstdir,
        "sbtdir":   self.sbtdir,
        }))


    def _module(self, name, src,
            xarchs=None, narchs=None,
            srcdir=None, dstdir=None,
            xflags=None, bflags=None, rflags=None, sbtflags=[],
            dbg=True):
        if not xarchs and not narchs:
            xarchs = self.xarchs
            narchs = self.narchs
        if not srcdir and not dstdir:
            srcdir = self.srcdir
            dstdir = self.dstdir
        bflags = cat(self.bflags, bflags)
        return Module(name, src,
                xarchs, narchs, srcdir, dstdir,
                xflags, bflags, rflags, sbtflags, dbg)


    def gen_basic(self):
        # tests
        rflags = "--tee"
        mods = [
            self._module("hello", "{}-hello.s", bflags="-C", rflags=rflags),
            self._module("argv", "argv.c", rflags="--args one two three " + rflags),
            self._module("mm", "mm.c", bflags='--cflags="-DROWS=4"', rflags=rflags),
            self._module("fp", "fp.c", rflags=rflags),
            self._module("printf", "printf.c", rflags=rflags),
            self._module("test", "rv32-test.s",
                xarchs=[], narchs=[RV32_LINUX], rflags=rflags),
        ]

        names = []
        for mod in mods:
            if GOPTS.cc == "gcc" and mod.name == "printf":
                continue
            self.append(mod.gen())
            names.append(mod.name)

        tests = [name + GenMake.test_suffix() for name in names]

        self.append("""\
### tests targets ###

.PHONY: tests
tests: x86-syscall-test x86-fp128 {names}

.PHONY: tests-run
tests-run: tests x86-syscall-test-run {tests}

""".format(**{"names": " ".join(names), "tests": " ".join(tests)}))


    def gen_utests(self):
        # utests
        self.append("### RV32 Translator unit tests ###\n\n")

        utests = [
            "load-store",
            "alu-ops",
            "branch",
            "fence",
            "system",
            "m",
            "f"
        ]

        narchs = [RV32_LINUX]
        xarchs = self.xarchs

        def sbtflags(test):
            if test == "branch":
                return ["-soft-float-abi"]
            else:
                return []

        xflags = "--sbtobjs syscall runtime counters"
        rflags = "--tee"

        for utest in utests:
            name = utest
            src = "rv32-" + name + ".s"
            mod = self._module(name, src, xarchs=xarchs, narchs=narchs,
                    xflags=xflags, rflags=rflags, sbtflags=sbtflags(name))
            self.append(mod.gen())

        utests_run = [utest + GenMake.test_suffix()
                    for utest in utests if utest != "system"]

        fmtdata = {
            "utests":       " ".join(utests),
            "utests_run":   " ".join(utests_run),
        }

        self.append("""\
.PHONY: utests
utests: {utests}

# NOTE removed system from utests to run (need MSR access and performance
# counters support (not always available in VMs))
utests-run: utests {utests_run}
\t@echo "All unit tests passed!"

""".format(**fmtdata))


    def gen_qemu(self):
        tests = [
            "add", "addi",
            "aiupc",
            "and", "andi",
            "beq", "bge", "bgeu", "blt", "bltu", "bne",
            "jal", "jalr",
            "lb", "lbu", "lhu", "lui", "lw",
            "or", "ori",
            "sb",
            "sll", "slli",
            "slt", "slti", "sltiu", "sltu",
            "sra", "srai", "srl", "srli",
            "sub",
            "sw",
            "xor", "xori",
        ]

        failing = []

        missing = [
            "csrrc", "csrrci",
            "csrrs", "csrrsi",
            "csrrw", "csrrwi",
            "ebreak", "ecall",
            "fence", "fence.i",
            "lh", "sh",
        ]

        status = "passing: {}\\n{}\\n".format(len(tests), " ".join(tests)) + \
                "failing: {}\\n{}\\n".format(len(failing), " ".join(failing)) + \
                "missing: {}\\n{}\\n".format(len(missing), " ".join(missing)) + \
                "total: {}\\n".format(len(tests + failing + missing))

        self.append("""\
### QEMU tests

.PHONY: rv32tests_status
rv32tests_status:
\t@printf "{status}"

""".format(**{"status":status}))

        dbg = False
        xarchs = self.xarchs
        narchs = [RV32_LINUX]
        qtests = path(DIR.top, "riscv-qemu-tests")
        incdir = qtests
        srcdir = path(qtests, "rv32i")
        dstdir = path(DIR.build, "test/qemu")
        bflags = '-C --sflags="-I {}"'.format(incdir)

        def sbtflags(test):
            if test == "aiupc":
                return ["-no-sym-bounds-check"]
            if test == "jalr":
                return ["-soft-float-abi"]
            return []

        for test in tests:
            name = test
            src = name + ".s"
            mod = self._module(name, src,
                    xarchs=xarchs, narchs=narchs,
                    srcdir=srcdir, dstdir=dstdir,
                    bflags=bflags, sbtflags=sbtflags(name))
            self.append(mod.gen())

        fmtdata = {
            "tests":        " ".join(tests),
            "dstdir":       dstdir,
            "teststgts":    " ".join([t + GenMake.test_suffix() for t in tests]),
        }

        self.append("""\
.PHONY: rv32tests
rv32tests: {tests}

rv32tests-run: rv32tests {teststgts}
\t@echo "All rv32tests passed!"

rv32tests-clean:
\trm -rf {dstdir}

""".format(**fmtdata))


    def gen_epilogue(self):
        fmtdata = {
            "top":      DIR.top,
            "srcdir":   self.srcdir,
            "dstdir":   self.dstdir,
            "measure":  path(DIR.auto, "measure.py"),
        }

        self.append("""\
### TEST targets

.PHONY: basic-test
basic-test:
\t$(MAKE) -C {top}/test all run

.PHONY: setmsr
setmsr:
\tsudo {top}/scripts/setmsr.sh

### matrix multiply test

mmm:
\t{measure} --no-perf --no-csv {dstdir} mm

### everything ###

clean:
\t$(MAKE) -C {top}/test clean
\trm -rf {dstdir}

.PHONY: almost-alltests
almost-alltests: basic-test tests-run utests-run mmm rv32tests-run

.PHONY: alltests
alltests: setmsr almost-alltests system-test
""".format(**fmtdata))


    def gen(self):
        self.gen_prologue()
        self.gen_basic()
        self.gen_utests()
        self.gen_qemu()
        self.gen_epilogue()
        return self.txt


if __name__ == "__main__":
    tests = Tests()
    txt = tests.gen()

    # write txt to Makefile
    with open("Makefile", "w") as f:
        f.write(txt)

