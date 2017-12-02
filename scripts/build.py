#!/usr/bin/python3

import argparse
import os
import subprocess
import sys

### config ###

# flags
MAKE_OPTS       = os.getenv("MAKE_OPTS", "-j9")
CFLAGS          = "-fno-rtti -fno-exceptions"
_O              = "-O3"

EMITLLVM        = "-emit-llvm -c {} -mllvm -disable-llvm-optzns".format(_O)
RV32_TRIPLE     = "riscv32-unknown-elf"


class Dir:
    def __init__(self):
        top             = os.environ["TOPDIR"]
        build_type      = os.getenv("BUILD_TYPE", "Debug")
        build_type_dir  = build_type.lower()
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
        self.nat_objs = ["syscall", "counters"]
        self.nat_obj  = lambda arch, name: \
            "{}/{}-{}.o".format(self.share_dir, arch.prefix, name)

SBT = Sbt()


class Tools:
    def __init__(self):
        self.cmake      = "cmake"
        self.run_sh     = DIR.scripts + "/run.sh"
        self.log        = self.run_sh + " --log"
        self.log_clean  = "rm -f log.txt"
        self.measure    = '{}; MODES="globals locals" {} {}/measure.py'.format(
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
            _cat("-relocation-model=static",
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


class Opts:
    def __init__(self):
        self.dbg = False
        self.asm = False
        self.clink = True
        # flags
        self.setsysroot = True
        self.cflags = ""
        self.sflags = ""
        self.ldflags = ""
        self.sbtflags = ""
        self.libs = ""


###

def shell(cmd, save_out=False):
    print(cmd)
    if save_out:
        cp = subprocess.run(cmd, shell=True, check=True,
            universal_newlines=True,
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        return cp.stdout
    else:
        subprocess.run(cmd, shell=True, check=True)

### rules ###

def _cat(*args):
    out = ""
    for arg in args:
        if not arg:
            pass
        elif len(arg) > 0:
            if len(out) == 0:
                out = arg
            else:
                out = out + " " + arg
    return out


# .c -> .bc
def _c2bc(arch, srcdir, dstdir, _in, out, opts):
    flags = _cat(
        arch.clang_flags,
        (arch.sysroot_flag if opts.setsysroot else ""),
        opts.cflags,
        EMITLLVM)

    ipath = srcdir + "/" + _in + ".c"
    opath = dstdir + "/" + out + ".bc"

    cmd = "{} {} {} -o {}".format(
        arch.clang, flags, ipath, opath)
    shell(cmd)


# *.bc -> .bc
def _lllink(arch, dir, ins, out):
    ipath = lambda mod: dir + "/" + mod + ".bc"
    opath = dir + "/" + out + ".bc"

    ipaths = ""
    for i in ins:
        ipaths = ipaths + " " + ipath(i)
    ipaths = ipaths.strip()

    cmd = "{} {} -o {}".format(
        TOOLS.link, ipaths, opath)
    shell(cmd)


# .bc -> .ll
def _dis(arch, dir, mod):
    ipath = dir + "/" + mod + ".bc"
    opath = dir + "/" + mod + ".ll"

    cmd = "{} {} -o {}".format(
        TOOLS.dis, ipath, opath)
    shell(cmd)


# opt
def _opt(arch, dir, _in, out):
    ipath = dir + "/" + _in + ".bc"
    opath = dir + "/" + out + ".bc"

    cmd = "{} {} {} -o {}".format(
        TOOLS.opt, TOOLS.opt_flags, ipath, opath)
    shell(cmd)


# *.c -> *.bc -> .bc
# (C2LBC: C to linked BC)
def _c2lbc(arch, srcdir, dstdir, ins, out, opts):
    if len(ins) > 1:
        ins2 = []
        for c in ins:
            o = c + "_bc1"
            ins2.append(o)

            _c2bc(arch, srcdir, dstdir, c, o, opts)
            _dis(arch, dstdir, o)

        _lllink(arch, dstdir, ins2, out)
    else:
        _c2bc(arch, srcdir, dstdir, ins[0], out, opts)
    _dis(arch, dstdir, out)


# .bc -> .s
def _bc2s(arch, dir, _in, out, opts):
    ipath = dir + "/" + _in + ".bc"
    opath = dir + "/" + out + ".s"

    flags = arch.llc_flags(opts)

    cmd = "{} {} {} -o {}".format(
        arch.llc, flags, ipath, opath)
    shell(cmd)
    if arch == RV32:
        cmd = 'sed -i "1i.option norelax" ' + opath
        shell(cmd)


# *.c -> .s
def _c2s(arch, srcdir, dstdir, ins, out, opts):
    _c2lbc(arch, srcdir, dstdir, ins, out, opts)

    opt1 = out + ".opt"
    opt2 = out + ".opt2"

    # opt; dis; opt; dis; .bc -> .s
    _opt(arch, dstdir, out, opt1)
    _dis(arch, dstdir, opt1)
    _opt(arch, dstdir, opt1, opt2)
    _dis(arch, dstdir, opt2)
    _bc2s(arch, dstdir, opt2, out, opts)


# .s -> .o
def _s2o(arch, srcdir, dstdir, _in, out, opts):
    ipath = srcdir + "/" + _in + ".s"
    opath = dstdir + "/" + out + ".o"

    flags = _cat(arch.as_flags, opts.sflags)

    cmd = _cat(arch._as, flags, "-c", ipath, "-o", opath)
    shell(cmd)


# *.o -> bin
def _link(arch, srcdir, dstdir, ins, out, opts):
    libs = opts.libs
    if opts.clink:
        tool = arch.gcc
        flags = arch.gcc_flags(opts)
        libs = _cat(libs, "-lm")
    else:
        tool = arch.ld
        flags = arch.ld_flags
    flags = _cat(flags, opts.ldflags)

    ipaths = [srcdir + "/" + i + ".o" for i in ins]
    opath = dstdir + "/" + out
    cmd = _cat(tool, flags, " ".join(ipaths), "-o", opath, libs)
    shell(cmd)


# *.s -> bin
def _snlink(arch, srcdir, dstdir, ins, out, opts):
    if len(ins) == 1:
        _s2o(arch, srcdir, dstdir, ins[0], out, opts)
        _link(arch, dstdir, dstdir, [out], out, opts)
    else:
        for i in ins:
            _s2o(arch, srcdir, dstdir, i, i, opts)
        _link(arch, dstdir, dstdir, ins, out, opts)


# *.c -> bin
def _cnlink(arch, srcdir, dstdir, ins, out, opts):
    _c2s(arch, srcdir, dstdir, ins, out, opts)
    _s2o(arch, dstdir, dstdir, out, out, opts)
    _link(arch, dstdir, dstdir, [out], out, opts)


# *.c/s -> bin
def build(arch, srcdir, dstdir, ins, out, opts):
    # create dstdir if needed
    shell("mkdir -p " + dstdir)

    if opts.asm:
        _snlink(arch, srcdir, dstdir, ins, out, opts)
    else:
        _cnlink(arch, srcdir, dstdir, ins, out, opts)


# run
def run(arch, dir, prog, args, save_out):
    path = dir + "/" + prog

    cmd = _cat(arch.run, path, " ".join(args))
    out = shell(cmd, save_out)
    if save_out:
        with open(path + ".out", "w") as f:
            f.write(out)


# clean
def clean(dir, mod, clean_s):
    if os.path.isdir(dir) and os.path.exists(dir):
        os.chdir(dir)

        s = ("{0} {0}.o {1}{0}.ll {0}.opt.bc {0}.opt.ll " + \
            "{0}.opt2.bc {0}.opt2.ll {0}.out").format(
                dir + "/" + mod, dir + "/" + mod + ".s " if clean_s else "")
        files = s.split()
        for f in files:
            if os.path.exists(f):
                print("rm " + f)
                os.remove(f)


# .o -> .bc
def _translate_obj(arch, dir, _in, out, opts):
    flags = _cat(SBT.flags, opts.sbtflags)

    ipath = dir + "/" + _in + ".o"
    opath = dir + "/" + out + ".bc"
    log = DIR.top + "/junk/" + out + ".log"

    cmd = "riscv-sbt {} {} -o {} >{} 2>&1".format(
        flags, ipath, opath, log)
    shell(cmd)


# .o -> bin
def translate(arch, srcdir, dstdir, _in, out, opts):
    _translate_obj(arch, dstdir, _in, out, opts)

    _dis(arch, dstdir, out)

    if opts.dbg:
        _bc2s(arch, dstdir, out, out, opts)
    else:
        opt1 = out + ".opt"
        _opt(arch, dstdir, out, opt1)
        _dis(arch, dstdir, opt1)
        _bc2s(arch, dstdir, opt1, out, opts)


### sbt test ###

def main():
    parser = argparse.ArgumentParser(description="build/run programs")
    parser.add_argument("-C", action='store_false', help="don't link with C libs")
    parser.add_argument("--asm", action='store_true', help="treat inputs as assembly")
    parser.set_defaults(cmd=None)
    subparsers = parser.add_subparsers(help="sub-command help")

    build_parser = subparsers.add_parser("build", help="build program")
    build_parser.add_argument("arch")
    build_parser.add_argument("srcdir")
    build_parser.add_argument("dstdir")
    build_parser.add_argument("out")
    build_parser.add_argument("ins", nargs="+", metavar="in")
    build_parser.set_defaults(cmd="build")
    build_parser.add_argument("--libs")
    build_parser.add_argument("--cflags")
    build_parser.add_argument("--sflags")
    build_parser.add_argument("--syscall", action="store_true",
        help="link with syscall translation obj")
    build_parser.add_argument("--counters", action="store_true",
        help="link with counters translation obj")

    run_parser = subparsers.add_parser("run", help="run program")
    run_parser.add_argument("arch")
    run_parser.add_argument("dir")
    run_parser.add_argument("prog")
    run_parser.add_argument("args", nargs="*", metavar="arg")
    run_parser.add_argument("--save-output", action="store_true")
    run_parser.set_defaults(cmd="run")

    clean_parser = subparsers.add_parser("clean", help="clean build output files")
    clean_parser.add_argument("dir")
    clean_parser.add_argument("mod")
    clean_parser.set_defaults(cmd="clean")

    translate_parser = subparsers.add_parser("translate", help="translate obj")
    translate_parser.add_argument("arch")
    translate_parser.add_argument("srcdir")
    translate_parser.add_argument("dstdir")
    translate_parser.add_argument("out")
    translate_parser.add_argument("_in")
    translate_parser.add_argument("flags", nargs="*", metavar="flag")
    translate_parser.set_defaults(cmd="translate")

    args = parser.parse_args()

    if not args.cmd:
        sys.exit("ERROR: no sub-command specified")

    # arch map
    arch = {
        "rv32"          : RV32,
        "rv32-linux"    : RV32_LINUX,
        "x86"           : X86,
    }

    # global opts
    opts = Opts()
    opts.clink = args.C
    opts.asm = args.asm

    # build
    if args.cmd == "build":
        arch = arch[args.arch]
        libs = args.libs
        if arch != RV32 and arch != RV32_LINUX:
            # syscall
            if args.syscall:
                libs = _cat(libs, SBT.nat_obj(arch, "syscall"))
            # counters
            if args.counters:
                libs = _cat(libs, SBT.nat_obj(arch, "counters"))

        opts.cflags = _cat(opts.cflags, args.cflags)
        opts.sflags = _cat(opts.sflags, args.sflags)
        opts.libs = libs

        build(arch, args.srcdir, args.dstdir, args.ins, args.out, opts)

    # run
    elif args.cmd == "run":
        save = args.save_output
        run(arch[args.arch], args.dir, args.prog, args.args, save)

    # clean
    elif args.cmd == "clean":
        clean(args.dir, args.mod, not args.asm)

    # translate
    elif args.cmd == "translate":
        arch = arch[args.arch]
        # libc
        flags = " ".join(args.flags)
        o = "-dont-use-libc"
        if not opts.clink and flags.find(o) == -1:
            flags = _cat(flags, o)

        # set flags
        opts.sbtflags = flags

        # translate
        translate(arch, args.srcdir, args.dstdir, args._in,
                args.out, opts)

main()
