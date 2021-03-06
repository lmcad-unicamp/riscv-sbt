#!/usr/bin/env python3

from auto.config import ARCH, GOPTS, RV32_RUN_ARGV
from auto.utils import cat, path, shell

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
        save_out = len(out) > 0 and self.arch.can_run()

        argv_sep = None
        if arch.is_rv32():
            argv_sep = RV32_RUN_ARGV

        # XXX hack
        if arch.is_rv32() and GOPTS.rv32 == "ovp" and prog == "rv32-blowfish":
            exp_rc = 0

        cmd = cat(arch.run, ppath, argv_sep, " ".join(args))
        if tee:
            cmd = "/bin/bash -c " + shlex.quote(cmd + " | tee " + opath +
                  "; exit ${PIPESTATUS[0]}")
            self._run_native(cmd, save_out, bin, exp_rc)
        else:
            try:
                f = None
                if save_out:
                    f = open(opath, "wb" if bin else "w")
                sout = self._run_native(cmd, save_out, bin, exp_rc)
                if save_out:
                    f.write(sout)
            finally:
                if f:
                    f.close()


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
