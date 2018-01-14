#!/usr/bin/env python3

from auto.config import *
from auto.genmake import *

class Tests():
    def __init__(self):
        self.narchs = [RV32, X86]
        self.xarchs = [(RV32, X86)]
        self.srcdir = path(DIR.top, "test/sbt")
        self.dstdir = path(DIR.build, "test/sbt")
        self.txt = None


    def gen_prologue(self):
        self.txt = """\
all: elf tests utests

### elf ###

CXX      = g++
CXXFLAGS = -m32 -Wall -Werror -g -std=c++11 -pedantic
LDFLAGS  = -m32

{dstdir}:
	mkdir -p $@

{dstdir}/elf.o: elf.cpp elf.hpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

{dstdir}/elf: {dstdir} {dstdir}/elf.o
	$(CXX) $(LDFLAGS) -o $@ {dstdir}/elf.o

.PHONY: elf
elf: {dstdir}/elf

### x86-syscall-test ###

.PHONY: x86-syscall-test
x86-syscall-test: {dstdir}/x86-syscall-test

{dstdir}/x86-syscall-test: {srcdir}/x86-syscall-test.s
	{build} --arch x86 --srcdir {srcdir} --dstdir {dstdir} x86-syscall-test.s -o x86-syscall-test

.PHONY: x86-syscall-test-run
x86-syscall-test-run:
	{run} --arch x86 --dir {dstdir} x86-syscall-test

""".format(**{
        "build":    TOOLS.build,
        "run":      TOOLS.run,
        "srcdir":   self.srcdir,
        "dstdir":   self.dstdir,
        })


    def gen_basic(self):
        dbg = True

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
            xflags = cat(mod.xflags, "--dbg" if dbg else '')
            bflags = cat(mod.bflags, "--dbg" if dbg else '')
            rflags = mod.rflags
            self.txt = self.txt + do_mod(
                self.narchs, self.xarchs,
                name, src,
                self.srcdir, self.dstdir,
                xflags, bflags, rflags)

        names = [mod.name for mod in mods]
        tests = [name + "-test" for name in names]

        self.txt = self.txt + """\
### tests targets ###

.PHONY: tests
tests: x86-syscall-test {names}

.PHONY: tests-run
tests-run: tests x86-syscall-test-run {tests}

""".format(**{"names": " ".join(names), "tests": " ".join(tests)})

        # rv32-test
        mod = Module("test", "rv32-test.s", rflags=rflags)
        self.txt = self.txt + do_mod([RV32], [(RV32, X86)],
            mod.name, mod.src, self.srcdir, self.dstdir,
            mod.xflags, mod.bflags, mod.rflags)


    def gen_utests(self):
        dbg = True

        # utests
        self.txt = self.txt + "### RV32 Translator unit tests ###\n\n"

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
        rflags = "-o {}.out --tee"

        if dbg:
            bflags = cat(bflags, "--dbg")
            xflags = cat(bflags, "--dbg")

        for mod in mods:
            name = mod
            src = "rv32-" + name + ".s"
            self.txt = self.txt + do_mod(
                narchs, self.xarchs,
                name, src,
                self.srcdir, self.dstdir,
                xflags, bflags, rflags)

        utests = [mod + "-test" for mod in mods if mod != "system"]

        fmtdata = {
            "mods":     " ".join(mods),
            "utests":   " ".join(utests),
        }

        self.txt = self.txt + """\
.PHONY: utests
utests: {mods}

# NOTE removed system from utests to run (need MSR access and performance
# counters support (not always available in VMs))
utests-run: utests {utests}
	@echo "All unit tests passed!"

""".format(**fmtdata)


    def gen_qemu(self):
        tests = [
            "add",
            "addi",
            "aiupc",
            "and",
            "andi",
            "beq",
            "bge",
            "bgeu",
            "blt",
            "bltu",
            "bne",
            "jal",
            "jalr",
            "lb",
            "lbu",
            "lhu",
            "lui",
            "lw",
            "or",
            "ori",
            "sb",
            "sll",
            "slli",
            "slt",
            "slti",
            "sltiu",
            "sltu",
            "sra",
            "srai",
            "srl",
            "srli",
            "sub",
            "sw",
            "xor",
            "xori",
        ]

        failing = []

        missing = [
            "csrrc",
            "csrrci",
            "csrrs",
            "csrrsi",
            "csrrw",
            "csrrwi",
            "ebreak",
            "ecall",
            "fence",
            "fence.i",
            "lh",
            "sh",
        ]

        status = "passing: {}\\n{}\\n".format(len(tests), " ".join(tests)) + \
                "failing: {}\\n{}\\n".format(len(failing), " ".join(failing)) + \
                "missing: {}\\n{}\\n".format(len(missing), " ".join(missing)) + \
                "total: {}\\n".format(len(tests + failing + missing))

        self.txt = self.txt + """\
### QEMU tests

.PHONY: rv32tests_status
rv32tests_status:
	@printf "{status}"

""".format(**{"status":status})


        qtests = path(DIR.top, "riscv-qemu-tests")
        incdir = qtests
        srcdir = path(qtests, "rv32i")
        dstdir = path(DIR.build, "riscv-qemu-tests/rv32i")

        narchs = [RV32]
        xflags = "-C"
        bflags = '-C --sflags="-I {}"'.format(incdir)
        rflags = "-o {}.out"
        for test in tests:
            name = test
            src = name + ".s"
            self.txt = self.txt + do_mod(
                narchs, self.xarchs,
                name, src, srcdir, dstdir,
                xflags, bflags, rflags)

        fmtdata = {
            "tests":        " ".join(tests),
            "dstdir":       dstdir,
            "teststgts":    " ".join([t + "-test" for t in tests]),
        }

        self.txt = self.txt + """\
.PHONY: rv32tests
rv32tests: {tests}

rv32tests-run: rv32tests {teststgts}
	@echo "All rv32tests passed!"

rv32tests-clean:
	rm -rf {dstdir}

""".format(**fmtdata)


    def gen_epilogue(self):
        fmtdata = {
            "top":      DIR.top,
            "srcdir":   self.srcdir,
            "dstdir":   self.dstdir,
            "measure":  path(DIR.auto, "measure.py"),
        }

        self.txt = self.txt + """\
### TEST targets

.PHONY: basic-test
basic-test:
	$(MAKE) -C {top}/test all run

.PHONY: setmsr
setmsr:
	sudo {top}/scripts/setmsr.sh

### matrix multiply test

mmm:
	{measure} {dstdir} mm

### everything ###

clean: rv32tests-clean
	$(MAKE) -C {top}/test clean
	rm -rf {dstdir}

.PHONY: almost-alltests
almost-alltests: basic-test tests-run utests-run mmm rv32tests-run

.PHONY: alltests
alltests: setmsr almost-alltests system-test
""".format(**fmtdata)


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

