#!/usr/bin/env python3

from auto.build import Builder, BuildOpts
from auto.config import DIR, RV32_LINUX, SBT
from auto.utils import cat, chsuf, mkdir_if_needed, path, shell

import argparse

class Translator:
    def __init__(self, opts):
        self.opts = opts


    def _translate_obj(self, dir, obj, out):
        """ .o -> .bc """

        opts = self.opts
        arch = opts.arch
        ipath = path(dir, obj)
        opath = path(dir, out)
        flags = cat(SBT.flags, opts.xflags)

        if opts.dbg:
            # strip arch prefix
            prefix = arch.add_prefix("")
            prefix = RV32_LINUX.add_prefix(prefix)
            base = out[len(prefix):]
            # strip suffix
            base = chsuf(base, "")
            # strip mode
            p = base.rfind("-")
            if p != -1:
                base = base[:p]
            # add rv32 prefix and .a2s suffix
            a2s = RV32_LINUX.add_prefix(base) + ".a2s"
            flags = cat(flags, "-commented-asm", "-a2s", path(dir, a2s))

        logdir = DIR.top + "/junk"
        mkdir_if_needed(logdir)
        log = path(logdir, chsuf(out, ".log"))

        cmd = "riscv-sbt {} {} -o {} >{} 2>&1".format(
            flags, ipath, opath, log)
        shell(cmd)


    def translate(self):
        """ .o -> bin """

        opts = self.opts
        obj = opts.first
        dstdir = opts.dstdir
        out = opts.out

        bc = out + ".bc"
        s = out + ".s"

        # translate obj to .bc
        self._translate_obj(dstdir, obj, bc)
        # gen .ll
        bld = Builder(opts)
        bld.dis(dstdir, bc)

        # gen .s
        if opts.dbg:
            bld.bc2s(dstdir, bc, s)
        else:
            opt1 = out + ".opt.bc"
            bld.opt(dstdir, bc, opt1, printf_break=False)
            bld.dis(dstdir, opt1)
            bld.bc2s(dstdir, opt1, s)

        # build .s
        opts.asm = True
        opts.srcdir = dstdir
        opts.dstdir = dstdir
        opts.ins = [s]
        opts.out = out
        bld.build()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="translate obj")
    BuildOpts.add_to_parser(parser)
    args = parser.parse_args()

    # set xlator opts
    xltr = Translator(BuildOpts.parse(args))
    # translate
    xltr.translate()
