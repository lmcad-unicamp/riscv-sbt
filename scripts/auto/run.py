#!/usr/bin/env python3

from auto.config import *
from auto.utils import *

import argparse
import shlex

class Runner:
    def __init__(self, arch):
        self.arch = arch


    def _run_native(self, cmd, save_out, bin, exp_rc):
        if not self.arch.can_run():
            out = "Skipped (non-native): {}\n".format(cmd)
            print(out, end='')
            return out
        return shell(cmd, save_out, bin, exp_rc=exp_rc)


    # run
    def run(self, dir, prog, args, out, tee, bin, exp_rc):
        arch = self.arch
        ppath = path(dir, prog)
        opath = path(dir, out)
        save_out = len(out) > 0

        cmd = cat(arch.run, ppath, " ".join(args))
        if tee:
            cmd = "/bin/bash -c " + shlex.quote(cmd + " | tee " + opath +
                  "; exit ${PIPESTATUS[0]}")
            self._run_native(cmd, save_out, bin, exp_rc)
        else:
            if save_out and bin:
                sout = self._run_native(cmd, save_out, bin, exp_rc)
                with open(opath, "wb") as f:
                    f.write(sout)
            else:
                sout = self._run_native(cmd, save_out, bin, exp_rc)
                if save_out:
                    with open(opath, "w") as f:
                        f.write(sout)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="run program")
    parser.add_argument("prog")
    parser.add_argument("--args", nargs="*", metavar="arg", default=[])
    parser.add_argument("--arch", default="rv32")
    parser.add_argument("--dir", default=".")
    parser.add_argument("-o", help="save output to file", metavar="file",
            default="")
    parser.add_argument("--tee", action="store_true")
    parser.add_argument("--bin", action="store_true",
        help="use this flag if the output is a binary file")
    parser.add_argument("--exp-rc", type=int,
        help="expected return code", default=0)

    args = parser.parse_args()

    sargs = [arg.strip() for arg in args.args]
    runner = Runner(ARCH[args.arch])
    runner.run(args.dir, args.prog, sargs, args.o, args.tee,
            args.bin, args.exp_rc)
