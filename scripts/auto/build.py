#!/usr/bin/env python3

from auto.config import *
from auto.utils import *

import argparse
import os
import sys


class Opts:
    def __init__(self):
        self.dbg = None
        self.asm = None
        self.clink = None
        self.setsysroot = True
        # flags
        self.cflags = None
        self.sflags = None
        self.ldflags = None
        self.sbtflags = None
        #
        self._as = None


def _c2bc(arch, srcdir, dstdir, _in, out, opts):
    """ .c -> .bc """
    flags = cat(
        arch.clang_flags,
        (arch.sysroot_flag if opts.setsysroot else ""),
        opts.cflags,
        emit_llvm(opts))

    ipath = path(srcdir, _in)
    opath = path(dstdir, out)

    cmd = cat(arch.clang, flags, ipath, "-o", opath)
    shell(cmd)


def _lllink(arch, dir, ins, out):
    """ *.bc -> .bc """
    ipaths = [path(dir, i) for i in ins]
    opath = path(dir, out)

    cmd = cat(TOOLS.link, " ".join(ipaths), "-o", opath)
    shell(cmd)


def dis(arch, dir, mod):
    """ .bc -> .ll """
    ipath = path(dir, mod)
    opath = path(dir, chsuf(mod, ".ll"))

    cmd = cat(TOOLS.dis, ipath, "-o", opath)
    shell(cmd)


def opt(arch, dir, _in, out):
    """ opt """
    ipath = path(dir, _in)
    opath = path(dir, out)

    cmd = cat(TOOLS.opt, TOOLS.opt_flags, ipath, "-o", opath)
    shell(cmd)


def _c2lbc(arch, srcdir, dstdir, ins, out, opts):
    """ *.c -> *.bc -> .bc
    (C2LBC: C to linked BC)
    """
    ch2bc1 = lambda c: chsuf(c, '') + "_bc1.bc"

    if len(ins) == 1:
        _c2bc(arch, srcdir, dstdir, ins[0], out, opts)
    else:
        for c in ins:
            o = ch2bc1(c)
            _c2bc(arch, srcdir, dstdir, c, o, opts)
            dis(arch, dstdir, o)
        _lllink(arch, dstdir, [ch2bc1(c) for c in ins], out)
    dis(arch, dstdir, out)


def bc2s(arch, dir, _in, out, opts):
    """ .bc -> .s """
    ipath = path(dir, _in)
    opath = path(dir, out)
    flags = arch.llc_flags(opts)

    cmd = cat(arch.llc, flags, ipath, "-o", opath)
    shell(cmd)
    # workaround for align issue with as
    if opts._as == "as":
        if arch == RV32 or arch == RV32_LINUX:
            cmd = 'sed -i "1i.option norelax" ' + opath
            shell(cmd)


def _c2s(arch, srcdir, dstdir, ins, out, opts):
    """ *.c -> .s """
    bc = chsuf(out, ".bc")
    _c2lbc(arch, srcdir, dstdir, ins, bc, opts)

    if opts.dbg:
        bc2s(arch, dstdir, bc, out, opts)
    else:
        opt1 = chsuf(bc, ".opt.bc")
        opt2 = chsuf(bc, ".opt2.bc")

        # opt; dis; opt; dis; .bc -> .s
        opt(arch, dstdir, bc, opt1)
        dis(arch, dstdir, opt1)
        opt(arch, dstdir, opt1, opt2)
        dis(arch, dstdir, opt2)
        bc2s(arch, dstdir, opt2, out, opts)


def _s2o(arch, srcdir, dstdir, _in, out, opts):
    """ .s -> .o """
    ipath = path(srcdir, _in)
    opath = path(dstdir, out)

    if opts._as == "as":
        flags = cat(arch.as_flags, opts.sflags,
            "-g" if opts.dbg else '')
        cmd = cat(arch._as, flags, "-c", ipath, "-o", opath)
        shell(cmd)

    # llvm-mc
    # Unlike GNU as for RISC-V, llvm-mc don't screw up with alignment in the
    # generated object file, that would otherwise be fixed only later,
    # at link stage, after it's done with relaxing.
    else:
        cmd = cat(TOOLS.mc, "-arch=" + arch.march,
            "-mattr=" + arch.mattr, "-target-abi=ilp32d",
            "-assemble", "-filetype=obj", ipath, "-o", opath)
        shell(cmd)
        # temporary workaround: set double-float ABI flag in ELF object file
        shell(R"printf '\x04\x00\x00\x00' | " +
            "dd of=" + opath + " bs=1 seek=$((0x24)) count=4 conv=notrunc")



def _link(arch, srcdir, dstdir, ins, out, opts):
    """ *.o -> bin """
    if opts.clink:
        tool = arch.gcc
        flags = arch.gcc_flags(opts)
    else:
        tool = arch.ld
        flags = arch.ld_flags
    flags = cat(flags, opts.ldflags)
    if opts.clink:
        flags = cat(flags, "-lm")

    ipaths = [path(srcdir, i) for i in ins]
    opath = path(dstdir, out)
    cmd = cat(tool, " ".join(ipaths), "-o", opath, flags)
    shell(cmd)


def _snlink(arch, srcdir, dstdir, ins, out, opts):
    """ *.s -> bin """
    if len(ins) == 1:
        obj = out + ".o"
        _s2o(arch, srcdir, dstdir, ins[0], obj, opts)
        _link(arch, dstdir, dstdir, [obj], out, opts)
    else:
        for i in ins:
            _s2o(arch, srcdir, dstdir, i, chsuf(i, ".o"), opts)
        _link(arch, dstdir, dstdir, [_chsuf(i, ".o") for i in ins], out, opts)


def _cnlink(arch, srcdir, dstdir, ins, out, opts):
    """ *.c -> bin """
    s = out + ".s"
    o = out + ".o"

    _c2s(arch, srcdir, dstdir, ins, s, opts)
    _s2o(arch, dstdir, dstdir, s, o, opts)
    _link(arch, dstdir, dstdir, [o], out, opts)


def build(arch, srcdir, dstdir, ins, out, opts):
    """ *.c/*.s -> bin """
    # create dstdir if needed
    shell("mkdir -p " + dstdir)

    if opts.asm:
        _snlink(arch, srcdir, dstdir, ins, out, opts)
    else:
        _cnlink(arch, srcdir, dstdir, ins, out, opts)


### main ###

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="build program")
    parser.add_argument("ins", nargs="+", metavar="in")
    parser.add_argument("--arch", default="rv32")
    parser.add_argument("--srcdir", default=".")
    parser.add_argument("--dstdir", default=".")
    parser.add_argument("-o", help="default: 1st in without suffix")
    parser.add_argument("--cflags", help="compiler flags")
    parser.add_argument("--sflags", help="assembler flags")
    parser.add_argument("--ldflags", help="linker flags")
    parser.add_argument("-C", action='store_false', help="don't link with C libs")
    parser.add_argument("--dbg", action='store_true', help="build for debug")
    parser.add_argument("--sbtobjs", nargs="+", default=["syscall"],
        help="optional SBT native objs to link with")
    parser.add_argument("--as", dest="_as", help="assembler to use: as || mc",
        default="mc")

    args = parser.parse_args()

    # get args
    ins = args.ins
    first = ins[0]
    arch = ARCH[args.arch]
    srcdir = args.srcdir
    dstdir = args.dstdir
    out = args.o if args.o else chsuf(first, '')

    # set build opts
    opts = Opts()
    opts.clink = args.C
    # use first file's suffix to find out if we need to assemble
    # or compile
    opts.asm = first.endswith(".s")
    opts.dbg = args.dbg
    opts.cflags = args.cflags
    opts.sflags = args.sflags
    opts.ldflags = cat(*[SBT.nat_obj(arch, o) for o in args.sbtobjs])
    opts._as = args._as

    # build
    build(arch, srcdir, dstdir, ins, out, opts)
