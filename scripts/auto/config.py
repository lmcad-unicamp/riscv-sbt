#!/usr/bin/env python3

from auto.utils import cat, chsuf, path, shell

import os
import subprocess

### config ###

class GlobalOpts:
    def __init__(self):
        self.cc = "gcc"
        self.rvcc = "gcc"
        if self.rvcc == "gcc":
            self.rvabi = "ilp32d"
        else:
            self.rvabi = "ilp32"

        self.arm_copy = "ssh"
        #self.arm_copy = "adb"
        if self.arm_copy == "adb":
            self.static = True
        else:
            self.static = False

        self.printf_break = False

    def gcc(self):
        return self.cc == "gcc"

    def rv_gcc(self):
        return self.rvcc == "gcc"

    def rv_soft_float(self):
        return self.rvabi == "ilp32"

    def rv_hard_float(self):
        return self.rvabi == "ilp32d"

    def adb_copy(self):
        return self.arm_copy == "adb"

    def ssh_copy(self):
        return self.arm_copy == "ssh"

GOPTS = GlobalOpts()

# flags
CFLAGS          = "-fno-exceptions"
CLANG_CFLAGS    = "-fno-rtti"
_O              = "-O3"

emit_llvm       = lambda dbg, opt: \
    "-emit-llvm -c {}{} -mllvm -disable-llvm-optzns".format(
        "-g " if dbg else "", _O if opt else "-O0")
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
        self.auto               = self.scripts + "/auto"
        self.submodules         = top + "/submodules"

DIR = Dir()


class Sbt:
    def __init__(self):
        self.flags = "-debug"
        #self.flags = cat(self.flags,
        #        "-enable-fcsr",
        #        "-enable-fcvt-validation")
        self.share_dir = DIR.toolchain + "/share/riscv-sbt"
        self.modes = ["globals", "locals"]

    def nat_obj(self, arch, name, clink):
        is_rv32 = arch == RV32 or arch == RV32_LINUX
        is_runtime = name == "runtime"

        if is_rv32 and not is_runtime:
            return ""
        if is_runtime and not clink:
            if is_rv32:
                return ""
            name = "asm-runtime"
        if is_rv32 and is_runtime and GOPTS.rv_hard_float():
            suffix = "-hf"
        else:
            suffix = ""
        return "{}/{}-{}{}.o".format(self.share_dir, arch.prefix, name, suffix)


SBT = Sbt()


class Tools:
    def __init__(self):
        self.cmake      = "cmake"
        self.opt        = "opt"
        self.opt_flags  = lambda opt: _O if opt else "-O0"
        self.dis        = "llvm-dis"
        self.link       = "llvm-link"
        self.mc         = "llvm-mc"
        self.objdump    = "riscv64-unknown-linux-gnu-objdump"

        self.build      = path(DIR.auto, "build.py")
        self.run        = path(DIR.auto, "run.py")
        self.xlate      = path(DIR.auto, "xlate.py")
        self.measure    = path(DIR.auto, "measure.py")

TOOLS = Tools()

UNAME_M = shell("uname -m", save_out=True, quiet=True)
GCC7 = subprocess.run("gcc --version | head -n1 | grep 7.3 >/dev/null",
            shell=True, check=False).returncode == 0

RV32_MATTR_NORELAX = "-a,-c,+m,+f,+d"
RV32_MATTR      = RV32_MATTR_NORELAX + ",+relax"

class Arch:
    def __init__(self, name, prefix, triple, run, march, gccflags,
            clang_flags, sysroot, isysroot, llcflags, as_flags,
            ld_flags, mattr, gccoflags=None, gcc=None,
            rem_host=None, rem_topdir=None):

        self.name = name
        self.prefix = prefix
        self.triple = triple
        self.run = run
        self.march = march
        # gcc
        self.gcc = gcc if gcc else triple + "-gcc"
        self.gccflags = cat(CFLAGS, gccflags)
        self.gccoflags = gccoflags

        # clang
        self.clang = "clang"
        self.clang_flags = "{} {}".format(CFLAGS, clang_flags)
        self.sysroot = sysroot
        self.isysroot = isysroot
        self.sysroot_flag = "-isysroot {} -isystem {}".format(
            sysroot, isysroot)
        # llc
        self.llc = "llc"
        self.llcflags = llcflags

        # as
        self._as = triple + "-as"
        self.as_flags = as_flags
        # ld
        self.ld = triple + "-ld"
        self.ld_flags = ld_flags
        #
        self.mattr_var = mattr
        # remote
        self.rem_host = rem_host
        self.rem_topdir = rem_topdir


    def add_prefix(self, s):
        return self.prefix + "-" + s


    def src2objname(self, src):
        """ objname for source code input file """
        if src.startswith(self.prefix):
            name = src
        else:
            name = self.add_prefix(src)
        return chsuf(name, ".o")


    def out2objname(self, out):
        """ objname for output file """
        if out.startswith(self.prefix):
            name = out
        else:
            name = self.add_prefix(out)
        return name + ".o"


    def is_native(self):
        return UNAME_M.find(self.prefix) != -1


    def is_rv32(self):
        return self.prefix == "rv32"


    def is_arm(self):
        return self.prefix == "arm"


    def is_rv32_x86(self):
        return self.is_rv32() and UNAME_M.find("x86") != -1


    def can_run(self):
        return self.is_rv32_x86() or self.is_native()


    def get_remote_path(self, lpath):
        ltop = DIR.top
        p = lpath.find(ltop)
        if p != 0:
            raise Exception(
                "Expected to find local topdir at beginning of lpath")
        rpath = self.rem_topdir + lpath[len(ltop):]
        return rpath


    def mattr(self, relax=True):
        if self.is_rv32():
            return RV32_MATTR if relax else RV32_MATTR_NORELAX
        return self.mattr_var


LLC_STATIC      = "-relocation-model=static"
LLC_PIC         = "-relocation-model=pic"

PK32 = DIR.toolchain_release + "/" + RV32_TRIPLE + "/bin/pk"
RV32_SYSROOT    = DIR.toolchain_release + "/opt/riscv/" + RV32_TRIPLE
RV32_MARCH      = "riscv32"
RV32_LLC_FLAGS  = cat(LLC_STATIC,
        "-march=" + RV32_MARCH, "-mattr=" + RV32_MATTR)

RV32 = Arch(
        name="rv32",
        prefix="rv32",
        triple=RV32_TRIPLE,
        run="LD_LIBRARY_PATH={}/lib spike {}".format(
            DIR.toolchain_release, PK32),
        march=RV32_MARCH,
        gccflags="",
        clang_flags=cat(CLANG_CFLAGS, "--target=riscv32"),
        sysroot=RV32_SYSROOT,
        isysroot=RV32_SYSROOT + "/include",
        llcflags=RV32_LLC_FLAGS,
        as_flags="",
        ld_flags="",
        mattr=RV32_MATTR)


RV32_LINUX_SYSROOT  = DIR.toolchain_release + "/opt/riscv/sysroot"
RV32_LINUX_ABI          = GOPTS.rvabi
RV32_LINUX_GCC_FLAGS    = "-march=rv32g -mabi=" + RV32_LINUX_ABI
RV32_LINUX_AS_FLAGS     = "-march=rv32g -mabi=" + RV32_LINUX_ABI
RV32_LINUX_LD_FLAGS     = "-m elf32lriscv"

RV32_LINUX = Arch(
        name="rv32-linux",
        prefix="rv32",
        triple="riscv64-unknown-linux-gnu",
        run="qemu-riscv32 -L " + RV32_LINUX_SYSROOT,
        march=RV32_MARCH,
        gccflags=RV32_LINUX_GCC_FLAGS,
        clang_flags=cat(CLANG_CFLAGS, "--target=riscv32 -D__riscv_xlen=32"),
        sysroot=RV32_LINUX_SYSROOT,
        isysroot=RV32_LINUX_SYSROOT + "/usr/include",
        llcflags=RV32_LLC_FLAGS,
        as_flags=RV32_LINUX_AS_FLAGS,
        ld_flags=RV32_LINUX_LD_FLAGS,
        mattr=RV32_MATTR)


X86_TRIPLE      = "i686-linux-gnu" if GCC7 else "x86_64-linux-gnu"
X86_MARCH       = "x86"
X86_MATTR       = "avx"
X86_SYSROOT     = "/usr/i686-linux-gnu" if GCC7 else "/"
X86_ISYSROOT    = "/usr/i686-linux-gnu/include" if GCC7 else "/usr/include"
X86_GCC         = X86_TRIPLE + ("-gcc-7" if GCC7 else "-gcc")
X86_GCC_FLAGS   = "" if GCC7 else "-m32"

X86 = Arch(
        name="x86",
        prefix="x86",
        triple=X86_TRIPLE,
        run="",
        march=X86_MARCH,
        gcc=X86_GCC,
        gccflags=X86_GCC_FLAGS,
        gccoflags="-mfpmath=sse -m" + X86_MATTR,
        clang_flags=cat(CLANG_CFLAGS,
            "--target=x86_64-unknown-linux-gnu -m32"),
        sysroot=X86_SYSROOT,
        isysroot=X86_ISYSROOT,
        llcflags=cat(LLC_STATIC, "-march=" + X86_MARCH, "-mattr=" + X86_MATTR),
        as_flags="--32",
        ld_flags="-m elf_i386",
        mattr=X86_MATTR)


ARM_TRIPLE  = "arm-linux-gnueabihf"
ARM_PREFIX  = "arm"
ARM_SYSROOT = "/usr/arm-linux-gnueabihf"
ARM_MARCH   = "arm"
ARM_MATTR   = "armv7-a"
ARM_HOST    = os.getenv("ARM") if GOPTS.ssh_copy() else os.getenv("ADB")
ARM_TOPDIR  = (os.getenv("ARM_TOPDIR") if GOPTS.ssh_copy()
                else os.getenv("ADB_TOPDIR"))
ARM_GCC     = ARM_TRIPLE + ("-gcc-7" if GCC7 else "-gcc-6")

ARM = Arch(
        name=ARM_PREFIX,
        prefix=ARM_PREFIX,
        triple=ARM_TRIPLE,
        run="",
        march=ARM_MARCH,
        gcc=ARM_GCC,
        gccflags=cat("-march=" + ARM_MATTR, "-mfpu=vfpv3-d16"),
        clang_flags=cat(CLANG_CFLAGS,
            "--target=" + ARM_TRIPLE, "-fPIC"),
        sysroot=ARM_SYSROOT,
        isysroot=ARM_SYSROOT + "/include",
        llcflags=cat(LLC_PIC,
            "-march=" + ARM_MARCH,
            "-mattr=" + ARM_MATTR + ",vfp3,d16",
            "-float-abi=hard"),
        as_flags="",
        ld_flags="",
        mattr=ARM_MATTR,
        rem_host=ARM_HOST,
        rem_topdir=ARM_TOPDIR)


RV32_FOR_X86 = Arch(
        name="rv32-for-x86",
        prefix="rv32-for-x86",
        triple=RV32_LINUX.triple,
        run="",
        march=RV32_MARCH,
        gccflags=RV32_LINUX_GCC_FLAGS,
        clang_flags=cat(CLANG_CFLAGS, "--target=riscv32"),
        sysroot=X86_SYSROOT,
        isysroot=X86_ISYSROOT,
        llcflags=RV32_LLC_FLAGS,
        as_flags=RV32_LINUX_AS_FLAGS,
        ld_flags=RV32_LINUX_LD_FLAGS,
        mattr=RV32_MATTR)


# arch map
ARCH = {
    "arm"           : ARM,
    "rv32"          : RV32,
    "rv32-linux"    : RV32_LINUX,
    "x86"           : X86,
    "rv32-for-x86"  : RV32_FOR_X86,
}

###
