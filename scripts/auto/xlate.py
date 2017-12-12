#!/usr/bin/env python3

from auto.build import *

import argparse


def _translate_obj(arch, dir, _in, out, opts):
    """ .o -> .bc """
    ipath = path(dir, _in)
    opath = path(dir, out)
    flags = cat(SBT.flags, opts.sbtflags)

    log = DIR.top + "/junk/" + chsuf(out, ".log")

    cmd = "riscv-sbt {} {} -o {} >{} 2>&1".format(
        flags, ipath, opath, log)
    shell(cmd)


def translate(arch, srcdir, dstdir, _in, out, opts):
    """ .o -> bin """
    obj = _in
    bc = out + ".bc"
    s = out + ".s"

    _translate_obj(arch, dstdir, obj, bc, opts)
    dis(arch, dstdir, bc)

    if opts.dbg:
        bc2s(arch, dstdir, bc, s, opts)
    else:
        opt1 = out + ".opt.bc"
        opt(arch, dstdir, bc, opt1)
        dis(arch, dstdir, opt1)
        bc2s(arch, dstdir, opt1, s, opts)

    opts.asm = True
    build(arch, dstdir, dstdir, [s], out, opts)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="translate obj")
    parser.add_argument("_in", metavar="in", help="input object file")
    parser.add_argument("--arch", default="rv32")
    parser.add_argument("--srcdir", default=".")
    parser.add_argument("--dstdir", default=".")
    parser.add_argument("-o", help="output")
    parser.add_argument("--flags", nargs="+", metavar="flag")
    parser.add_argument("-C", action='store_false', help="don't link with C libs")
    parser.add_argument("--dbg", action='store_true', help="build for debug")
    parser.add_argument("--sbtobjs", nargs="+", default=["syscall"],
        help="optional SBT native objs to link with")

    args = parser.parse_args()

    # get args
    _in = args._in
    arch = ARCH[args.arch]
    srcdir = args.srcdir
    dstdir = args.dstdir
    out = args.o if args.o else chsuf(_in, '')

    # set xlate opts
    opts = Opts()
    opts.clink = args.C
    opts.dbg = args.dbg
    opts.cflags = ""
    opts.sflags = ""
    opts.ldflags = cat(*[SBT.nat_obj(arch, o) for o in args.sbtobjs])
    opts.sbtflags = cat(" ".join(args.flags).strip(),
            "-dont-use-libc" if not opts.clink else "")

    # translate
    translate(arch, srcdir, dstdir, _in, out, opts)
