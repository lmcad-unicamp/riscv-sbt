#!/usr/bin/env python3

import argparse

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



def xlate_main():
    parser = argparse.ArgumentParser(description="translate obj")
    parser.add_argument("_in", help="input object file")
    parser.add_argument("-o", help="output")
    parser.add_argument("--arch", help="default=rv32")
    parser.add_argument("--srcdir", help="default=.")
    parser.add_argument("--dstdir", help="default=.")
    parser.add_argument("--flags", nargs="+", metavar="flag")

    args = parser.parse_args()

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



if __name__ == "__main__":
    xlate_main()
