#!/usr/bin/python3

from auto.config import *

import argparse
import os
import subprocess
import sys
import time

TARGETS = [ 'x86' ]

class Options:
    def __init__(self, stdin, verbose):
        self.stdin = stdin
        self.verbose = verbose


class TestData:
    def __init__(self, bin):
        self.bin = bin

    def out(self, i):
        return "/tmp/" + self.bin + str(i) + ".out"


class Test:
    def __init__(self, target, dir, test, args, opts):
        self.target = target
        self.dir = dir
        self.test = test
        self.args = args
        self.opts = opts

        bin = target + '-' + test
        self.native = TestData(bin)
        self.rv32 = TestData("rv32-" + bin)

    def check_rc(self, rc):
        exp_rc = self.opts.exp_rc
        if rc != exp_rc:
            raise Exception("Failure! rc=" + str(rc) + " exp_rc=" + str(exp_rc))

    def run1(self, args, td, i):
        with open(td.out(i), 'wb') as fout:
            stdin = self.opts.stdin
            if stdin:
                with open(stdin, 'rb') as fin:
                    t0 = time.time()
                    cp = subprocess.call(args, stdin=fin, stdout=fout)
                    t1 = time.time()
                    self.check_rc(cp)
            else:
                t0 = time.time()
                cp = subprocess.call(args, stdout=fout)
                t1 = time.time()
                self.check_rc(cp)
        t = t1 - t0
        if self.opts.verbose:
            print("run #" + str(i) + ": time taken:", t)
        sys.stdout.flush()
        return t


    def run(self):
        def prep(td, mode=None):
            if self.opts.verbose or mode:
                print("measuring ", td.bin, ": mode=", mode, sep='')

            path = self.dir + '/' + td.bin
            if mode:
                path = path + '-' + mode
            args = [path]
            args.extend(self.args)
            if self.opts.verbose:
                print(" ".join(args))
            sys.stdout.flush()
            return args

        n = 10

        # native
        td = self.native
        args = prep(td)

        ntimes = []
        for i in range(n):
            t = self.run1(args, td, i)
            ntimes.append(t)

        # translated
        for mode in SBT.modes:
            td = self.rv32
            args = prep(td, mode)

            ttimes = []
            for i in range(n):
                t = self.run1(args, td, i)
                ttimes.append(t)

            # compare outputs
            if not self.opts.no_diff:
                for i in range(n):
                    rc = subprocess.call(
                        ["diff", self.native.out(i), self.rv32.out(i)])
                    if rc:
                        sys.exit("ERROR: "
                            "translated output differs from native (run #"
                                 + str(i) + ")")

            def median(vals):
                vals.sort()
                n = len(vals)
                if n % 2 == 0:
                    v1 = vals[n//2 -1]
                    v2 = vals[n//2]
                    return (v1 + v2) / 2
                else:
                    return vals[n//2 +1]

            nt = median(ntimes)
            tt = median(ttimes)
            oh = (tt - nt) / nt
            print("native time     =", nt)
            print("translated time =", tt)
            print("overhead        =", oh)


def measure(target, dir, test_name, args, opts):
    test = Test(target, dir, test_name, args, opts)
    test.run()


# dir test arg
def main(args):
    parser = argparse.ArgumentParser(description='Measure translation overhead')
    parser.add_argument('dir', type=str)
    parser.add_argument('test', type=str)
    parser.add_argument("--args", nargs="*", metavar="arg", default=[])
    parser.add_argument("--stdin", help="stdin redirection")
    parser.add_argument("-v", action="store_true", help="verbose")
    parser.add_argument("--exp-rc", type=int, default=0,
        help="expected return code")
    parser.add_argument("--no-diff", action="store_true")

    args = parser.parse_args()
    opts = Options(args.stdin, args.v)
    opts.exp_rc = args.exp_rc
    opts.no_diff = args.no_diff
    sargs = [arg.strip() for arg in args.args]

    # print("measuring", args.test)

    for target in TARGETS:
        measure(target, args.dir, args.test, sargs, opts)

main(sys.argv)
