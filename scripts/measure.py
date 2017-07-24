#!/usr/bin/python3

import subprocess
import sys
import time

TARGETS = [ 'x86' ]

class TestData:
    def __init__(self, bin):
        self.bin = bin
        self.out = "/tmp/" + bin + ".out"
        self.time = 0.0


class Test:
    def __init__(self, target, dir, test, args):
        self.target = target
        self.dir = dir
        self.test = test
        self.args = args

        bin = target + '-' + test
        self.native = TestData(bin)
        self.rv32 = TestData("rv32-" + bin)

    def run1(self, td):
        print("measuring", td.bin)
        path = self.dir + '/' + td.bin
        args = [path]
        args.extend(self.args)
        print(args)
        sys.stdout.flush()
        with open(td.out, 'w') as f:
            t0 = time.time()
            subprocess.check_call(args, stdout=f)
            t1 = time.time()
            t = t1 - t0
        print("time taken:", t)
        td.time = t


    def run(self):
        self.run1(self.native)
        self.run1(self.rv32)
        rc = subprocess.call(["diff", self.native.out, self.rv32.out])
        if rc:
            sys.exit("ERROR: translated output differs from native")
        nt = self.native.time
        rt = self.rv32.time
        oh = (rt - nt) / nt
        print("overhead=", oh, sep='')


def measure(target, dir, test_name, args):
    test = Test(target, dir, test_name, args)
    test.run()


# dir test arg
def main(args):
    if len(args) < 3:
        raise RuntimeError("wrong number of args")
    dir = args[1]
    test = args[2]
    print("measuring", test)
    for target in TARGETS:
        measure(target, dir, test, args[3:])

main(sys.argv)
