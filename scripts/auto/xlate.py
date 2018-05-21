#!/usr/bin/env python3

from auto.build import *
import auto.utils

import argparse


def _translate_obj(arch, dir, _in, out, opts):
    """ .o -> .bc """
    ipath = path(dir, _in)
    opath = path(dir, out)
    flags = cat(SBT.flags, opts.sbtflags)

    if opts.dbg:
        # strip arch prefix
        prefix = arch.add_prefix("")
        prefix = RV32_LINUX.add_prefix(prefix)
        base = out[len(prefix):]
        # strip suffix
        base = auto.utils.chsuf(base, "")
        # strip mode
        p = base.rfind("-")
        if p != -1:
            base = base[:p]
        # add rv32 prefix and .a2s suffix
        a2s = RV32_LINUX.add_prefix(base) + ".a2s"
        flags = cat(flags, "-commented-asm", "-a2s", path(dir, a2s))

    logdir = DIR.top + "/junk"
    auto.utils.mkdir_if_needed(logdir)
    log = path(logdir, chsuf(out, ".log"))

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
        opt(arch, dstdir, bc, opt1, opts, printf_break=False)
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
    parser.add_argument("--sbtobjs", nargs="+", default=["syscall", "runtime"],
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
    opts.ldflags = cat(*[SBT.nat_obj(arch, o, opts) for o in args.sbtobjs])
    opts.sbtflags = cat(" ".join(args.flags).strip(),
            "-dont-use-libc" if not opts.clink else "")

    # translate
    translate(arch, srcdir, dstdir, _in, out, opts)
