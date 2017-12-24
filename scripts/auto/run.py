#!/usr/bin/env python3

from auto.config import *
from auto.utils import *

import argparse
import shlex

# run
def run(arch, dir, prog, args, out, tee, bin):
    ppath = path(dir, prog)
    opath = path(dir, out)
    save_out = len(out) > 0

    cmd = cat(arch.run, ppath, " ".join(args))
    if tee:
        cmd = "/bin/bash -c " + shlex.quote(cmd + " | tee " + opath +
              "; exit ${PIPESTATUS[0]}")
        shell(cmd)
    else:
        if save_out and bin:
            sout = shell(cmd, save_out, bin)
            with open(opath, "wb") as f:
                f.write(sout)
        else:
            sout = shell(cmd, save_out)
            if save_out:
                with open(opath, "w") as f:
                    f.write(sout)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="run program")
    parser.add_argument("prog")
    parser.add_argument("args", nargs="*", metavar="arg")
    parser.add_argument("--arch", default="rv32")
    parser.add_argument("--dir", default=".")
    parser.add_argument("-o", help="save output to file", metavar="file",
            default="")
    parser.add_argument("--tee", action="store_true")
    parser.add_argument("--bin", action="store_true",
        help="use this flag if the output is a binary file")

    args = parser.parse_args()

    run(ARCH[args.arch], args.dir, args.prog, args.args, args.o, args.tee,
            args.bin)
