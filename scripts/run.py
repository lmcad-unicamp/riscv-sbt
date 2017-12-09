#!/usr/bin/env python3

import argparse

from config import *
from utils import *

# run
def run(arch, dir, prog, args, out):
    ppath = path(dir, prog)
    save_out = len(out) > 0

    cmd = cat(arch.run, ppath, " ".join(args))
    out = shell(cmd, save_out)
    if save_out:
        opath = path(dir, out)
        with open(opath, "w") as f:
            f.write(out)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="run program")
    parser.add_argument("prog")
    parser.add_argument("args", nargs="*", metavar="arg")
    parser.add_argument("--arch", default="rv32")
    parser.add_argument("--dir", default=".")
    parser.add_argument("-o", help="save output to file", metavar="file",
            default="")

    args = parser.parse_args()

    run(ARCH[args.arch], args.dir, args.prog, args.args, args.o)
