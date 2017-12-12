#!/usr/bin/env python3

from auto.genmake import *

fmtdata = {
    "top":              DIR.top,
    "sbt_test_srcdir":  path(DIR.top, "sbt/test"),
    "sbt_test_dstdir":  mpath(DIR.build, "sbt", DIR.build_type_dir, "test"),
    "measure":          path(DIR.auto, "measure.py"),
}

PROLOGUE = """\
### TEST targets

lc:
	cat {top}/sbt/*.h {top}/sbt/*.cpp {top}/sbt/*.s | wc -l

.PHONY: tests
tests:
	$(MAKE) -C {top}/test clean all run
	cd {sbt_test_srcdir} && ./genmake.py
	$(MAKE) -C {sbt_test_srcdir} clean alltests-run

### rv32-system test

.PHONY: system-test
system-test:
	rm -f {sbt_test_dstdir}/rv32-*system*
	sudo {top}/scripts/setmsr.sh
	$(MAKE) -C {sbt_test_srcdir} system system-test

### matrix multiply test

mmm:
	{measure} {sbt_test_dstdir} mm

.PHONY: mmtest
mmtest: sbt
	rm -f {sbt_test_dstdir}/*-mm*
	$(MAKE) -C {sbt_test_srcdir} mm
	$(MAKE) -f {top}/make/tests.mk mmm

""".format(**fmtdata)

EPILOGUE = """\
### everything ###

.PHONY: alltests
alltests: tests system-test rv32tests-clean-run mmm
"""

if __name__ == "__main__":
    txt = PROLOGUE

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

    fmtdata = {
        "status":   status,
    }

    txt = txt + """\
### QEMU tests

.PHONY: rv32tests_status
rv32tests_status:
	@printf "{status}"

""".format(**fmtdata)


    qtests = path(DIR.top, "riscv-qemu-tests")
    incdir = qtests
    srcdir = path(qtests, "rv32i")
    dstdir = path(DIR.build, "riscv-qemu-tests/rv32i")

    narchs = ["rv32"]
    xarchs = ["rv32-x86"]
    xflags = "-C"
    bflags = '-C --sflags="-I {}"'.format(incdir)
    rflags = "-o {}.out"
    for test in tests:
        name = test
        src = name + ".s"
        txt = txt + do_mod(narchs, xarchs, name, src, srcdir, dstdir,
                xflags, bflags, rflags)


    fmtdata = {
        "tests":        " ".join(tests),
        "dstdir":       dstdir,
        "teststgts":    " ".join([t + "-test" for t in tests]),
    }

    txt = txt + """\
.PHONY: rv32tests
rv32tests: {tests}

rv32tests-run: rv32tests {teststgts}
	@echo "All rv32tests passed!"

rv32tests-clean:
	rm -rf {dstdir}

rv32tests-clean-run: rv32tests-clean rv32tests-run

""".format(**fmtdata)

    txt = txt + EPILOGUE

    # write txt
    opath = mpath(DIR.top, "make", "tests.mk")
    with open(opath, "w") as f:
        f.write(txt)

