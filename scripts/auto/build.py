#!/usr/bin/env python3

from auto.config import *
from auto.utils import *

import argparse
import os
import re
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


def opt(arch, dir, _in, out, opts, printf_break):
    """ opt """
    ipath = path(dir, _in)
    opath = path(dir, out)

    flags = TOOLS.opt_flags(opts)
    if printf_break:
        flags = cat(flags,
            "-load", "libPrintfBreak.so", "-printf-break")

    cmd = cat(TOOLS.opt, flags, ipath, "-o", opath)
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
        opt(arch, dstdir, bc, bc, opts, printf_break=True)
        dis(arch, dstdir, bc)
        bc2s(arch, dstdir, bc, out, opts)
    else:
        opt1 = chsuf(bc, ".opt.bc")
        opt2 = chsuf(bc, ".opt2.bc")

        # opt; dis; opt; dis; .bc -> .s
        opt(arch, dstdir, bc, opt1, opts, printf_break=True)
        dis(arch, dstdir, opt1)
        opt(arch, dstdir, opt1, opt2, opts, printf_break=False)
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
        # XXX temporary workaround for x86 debugging
        #     (upstream llvm-mc is not generating debug info)
        #if opts.dbg and out.find('x86-') >= 0:
        #    mc = "/usr/bin/llvm-mc-3.9"
        #else:
        #    mc = TOOLS.mc
        mc = TOOLS.mc

        # preserve rv32/x86 source code for debug, by not mixing in
        # assembly source together
        if opts.dbg and out.startswith('rv32-x86-'):
            flags = "-g"
        else:
            flags = ""

        flags = cat(
            flags,
            opts.sflags,
            "-arch=" + arch.march,
            "-mattr=" + arch.mattr)
        cmd = cat(mc, flags,
            "-assemble", "-filetype=obj",
            ipath, "-o", opath)
        shell(cmd)
        # temporary workaround: set double-float ABI flag in ELF object file
        #shell(R"printf '\x04\x00\x00\x00' | " +
        #    "dd of=" + opath + " bs=1 seek=$((0x24)) count=4 conv=notrunc" +
        #    " >/dev/null 2>&1")


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


def addr2src(dstdir, src, out):
    with open(path(dstdir, src)) as fin:
        with open(path(dstdir, out + ".a2s"), "w") as fout:
            patt = re.compile('([ 0-9a-f]{8}):\t')
            lns = []
            for ln in fin:
                m = patt.match(ln)
                if m:
                    if lns:
                        addr = int(m.group(1), 16)
                        # print("[{:04X}]:".format(addr))
                        fout.write("[{:04X}]:\n".format(addr))
                        for l in lns:
                            #print(l, end='')
                            fout.write(l)
                        lns = []
                else:
                    lns.append(ln)


def _gen_list(dstdir, out):
    ipath = path(dstdir, out + ".o")
    ofile = out + ".list"
    opath = path(dstdir, ofile)

    cmd = cat(TOOLS.objdump, "-S",
        ipath, ">", opath)
    shell(cmd)

    addr2src(dstdir, ofile, out)


def build(arch, srcdir, dstdir, ins, out, opts):
    """ *.c/*.s -> bin """
    # create dstdir if needed
    shell("mkdir -p " + dstdir)

    if opts.asm:
        _snlink(arch, srcdir, dstdir, ins, out, opts)
    else:
        _cnlink(arch, srcdir, dstdir, ins, out, opts)

    if opts.dbg and arch == RV32_LINUX:
        _gen_list(dstdir, out)


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
    parser.add_argument("--sbtobjs", nargs="+", default=["syscall", "runtime"],
        help="optional SBT native objs to link with")
    parser.add_argument("--as", dest="_as", help="assembler to use: as || mc",
        default="mc")
    parser.add_argument("--addr2src", action='store_true')

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
    opts.ldflags = cat(*[SBT.nat_obj(arch, o, opts) for o in args.sbtobjs])
    opts._as = args._as

    # addr2src
    if args.addr2src:
        addr2src(dstdir, first, out)
    # build
    else:
        build(arch, srcdir, dstdir, ins, out, opts)
