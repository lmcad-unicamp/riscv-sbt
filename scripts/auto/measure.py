#!/usr/bin/python3

from auto.config import *

import argparse
import os
import subprocess
import sys
import time

TARGETS = [ 'x86' ]

class TestData:
    def __init__(self, bin):
        self.bin = bin

    def out(self, i):
        return "/tmp/" + self.bin + str(i) + ".out"


class Test:
    def __init__(self, target, dir, test, args):
        self.target = target
        self.dir = dir
        self.test = test
        self.args = args

        bin = target + '-' + test
        self.native = TestData(bin)
        self.rv32 = TestData("rv32-" + bin)

    def run1(self, args, td, i):
        with open(td.out(i), 'w') as f:
            t0 = time.time()
            subprocess.check_call(args, stdout=f)
            t1 = time.time()
            t = t1 - t0
        print("run #" + str(i) + ": time taken:", t)
        sys.stdout.flush()
        return t


    def run(self):
        def prep(td, mode=None):
            print("measuring", td.bin, "mode:", mode)

            path = self.dir + '/' + td.bin
            if mode:
                path = path + '-' + mode
            args = [path]
            args.extend(self.args)
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
            for i in range(n):
                rc = subprocess.call(["diff", self.native.out(i), self.rv32.out(i)])
                if rc:
                    sys.exit("ERROR: translated output differs from native (run #"
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


def measure(target, dir, test_name, args):
    test = Test(target, dir, test_name, args)
    test.run()


# dir test arg
def main(args):
    parser = argparse.ArgumentParser(description='Measure translation overhead')
    parser.add_argument('dir', type=str)
    parser.add_argument('test', type=str)
    parser.add_argument('args', metavar='arg', type=str, nargs='*')

    args = parser.parse_args()

    print("measuring", args.test)

    for target in TARGETS:
        measure(target, args.dir, args.test, args.args)

main(sys.argv)
