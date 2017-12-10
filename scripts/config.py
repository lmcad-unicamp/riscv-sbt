#!/usr/bin/env python3

import os

from utils import *

### config ###

# flags
CFLAGS          = "-fno-rtti -fno-exceptions"
_O              = "-O3"

EMITLLVM        = "-emit-llvm -c {} -mllvm -disable-llvm-optzns".format(_O)
RV32_TRIPLE     = "riscv32-unknown-elf"


class Dir:
    def __init__(self):
        top             = os.environ["TOPDIR"]
        build_type      = os.getenv("BUILD_TYPE", "Debug")
        build_type_dir  = build_type.lower()
        self.build_type_dir = build_type_dir
        toolchain       = top + "/toolchain"

        self.top                = top
        self.log                = top + "/junk"
        self.toolchain_release  = toolchain + "/release"
        self.toolchain_debug    = toolchain + "/debug"
        self.toolchain          = toolchain + "/" + build_type_dir
        self.remote             = top + "/remote"
        self.build              = top + "/build"
        self.patches            = top + "/patches"
        self.scripts            = top + "/scripts"
        self.submodules         = top + "/submodules"

DIR = Dir()


class Sbt:
    def __init__(self):
        self.flags = "-x"
        self.share_dir = DIR.toolchain + "/share/riscv-sbt"
        self.nat_obj  = lambda arch, name: \
            "{}/{}-{}.o".format(self.share_dir, arch.prefix, name) \
            if arch != RV32 and arch != RV32_LINUX else ""
        self.modes = ["globals", "locals"]

SBT = Sbt()


class Tools:
    def __init__(self):
        self.cmake      = "cmake"
        self.run_sh     = DIR.scripts + "/run.sh"
        self.log        = self.run_sh + " --log"
        self.log_clean  = "rm -f log.txt"
        self.measure    = '{}; {} {}/measure.py'.format(
            self.log_clean, self.log, DIR.scripts)
        self.opt        = "opt"
        self.opt_flags  = _O #-stats
        self.dis        = "llvm-dis"
        self.link       = "llvm-link"

TOOLS = Tools()


class Arch:
    def __init__(self, prefix, triple, run, march, gcc_flags,
            clang_flags, sysroot, isysroot, llc_flags, as_flags,
            ld_flags):

        self.prefix = prefix
        self.triple = triple
        self.run = run
        self.march = march
        # gcc
        self.gcc = triple + "-gcc"
        self.gcc_flags = lambda opts: \
            "-static {} {} {}".format(
                ("-O0 -g" if opts.dbg else _O),
                CFLAGS,
                gcc_flags)
        # clang
        self.clang = "clang"
        self.clang_flags = "{} {}".format(CFLAGS, clang_flags)
        self.sysroot = sysroot
        self.isysroot = isysroot
        self.sysroot_flag = "-isysroot {} -isystem {}".format(
            sysroot, isysroot)
        # llc
        self.llc = "llc"
        self.llc_flags = lambda opts: \
            cat("-relocation-model=static",
                ("-O0" if opts.dbg else _O),
                llc_flags)
        # as
        self._as = triple + "-as"
        self.as_flags = as_flags
        # ld
        self.ld = triple + "-ld"
        self.ld_flags = ld_flags


PK32 = DIR.toolchain_release + "/" + RV32_TRIPLE + "/bin/pk"
RV32_SYSROOT    = DIR.toolchain_release + "/opt/riscv/" + RV32_TRIPLE
RV32_MARCH      = "riscv32"
RV32_LLC_FLAGS  = "-march=" + RV32_MARCH + " -mattr=+m"

RV32 = Arch(
        "rv32",
        RV32_TRIPLE,
        "LD_LIBRARY_PATH={}/lib spike {}".format(
            DIR.toolchain_release, PK32),
        RV32_MARCH,
        "",
        "--target=riscv32",
        RV32_SYSROOT,
        RV32_SYSROOT + "/include",
        RV32_LLC_FLAGS,
        "",
        "")


RV32_LINUX_SYSROOT  = DIR.toolchain_release + "/opt/riscv/sysroot"
RV32_LINUX_ABI      = "ilp32"

RV32_LINUX = Arch(
        "rv32",
        "riscv64-unknown-linux-gnu",
        "qemu-riscv32",
        RV32_MARCH,
        "-march=rv32g -mabi=" + RV32_LINUX_ABI,
        "--target=riscv32 -D__riscv_xlen=32",
        RV32_LINUX_SYSROOT,
        RV32_LINUX_SYSROOT + "/usr/include",
        RV32_LLC_FLAGS,
        "-march=rv32g -mabi=" + RV32_LINUX_ABI,
        "-m elf32lriscv")


X86_MARCH = "x86"

X86 = Arch(
        "x86",
        "x86_64-linux-gnu",
        "",
        X86_MARCH,
        "-m32",
        "--target=x86_64-unknown-linux-gnu -m32",
        "/",
        "/usr/include",
        "-march=" + X86_MARCH + " -mattr=avx",
        "--32",
        "-m elf_i386")


# arch map
ARCH = {
    "rv32"          : RV32,
    "rv32-linux"    : RV32_LINUX,
    "x86"           : X86,
}

###
