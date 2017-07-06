#!/usr/bin/python3

import subprocess
import sys
import time

TARGETS = [ 'x86', 'rv32-x86' ]

def measure(dir, test, target):
    bin = dir + '/' + target + '-' + test
    print("measuring", target)
    t0 = time.time()
    subprocess.call([bin])
    t1 = time.time()
    print("time taken:", t1 - t0)

def main(args):
    if len(args) != 3:
        raise RuntimeError("wrong number of args")
    dir = args[1]
    test = args[2]
    print("measuring", test)
    for target in TARGETS:
        measure(dir, test, target)

main(sys.argv)
