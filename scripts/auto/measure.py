#!/usr/bin/python3

from auto.config import *
from auto.utils import path

import argparse
import math
import numpy as np
import os
import re
import subprocess
import sys
import time

class Options:
    def __init__(self, stdin, verbose, exp_rc):
        self.stdin = stdin
        self.verbose = verbose
        self.exp_rc = exp_rc


class Program:
    def __init__(self, dir, basename, arch, mode, args, opts):
        self.name = arch + '-' + basename
        if mode:
            self.name = self.name + '-' + mode
        else:
            mode = 'native'
        self.mode = mode
        self.args = [path(dir, self.name)] + args
        self.opts = opts
        self.times = []
        self.lcperfs = []

        if self.opts.verbose:
            print(" ".join(self.args))


    def _out(self, i):
        return "/tmp/" + self.name + str(i) + ".out"


    def _perf_out(self, i):
        return "/tmp/" + self.name + str(i) + ".perf.out"


    def _check_rc(self, rc):
        exp_rc = self.opts.exp_rc
        if rc != exp_rc:
            raise Exception("Failure! rc=" + str(rc) + " exp_rc=" + str(exp_rc))


    def _extract_perf_time(self, out):
        patt = re.compile(" *([^ ]+) seconds time elapsed")
        with open(out, 'r') as f:
            for l in f:
                m = patt.match(l)
                if m:
                    return float(m.group(1))
            else:
                raise Exception("Failed to extract perf time!")


    def perf_libc(self, verbose=False, freq=None):
        args = ["perf", "record", "-q"]
        if freq:
            args.extend(["-F", str(freq)])
        args.extend(self.args)
        if verbose:
            print(" ".join(args))

        fin = None
        try:
            stdin = self.opts.stdin
            if stdin:
                fin = open(stdin, 'rb')
            stdout = subprocess.DEVNULL

            cp = subprocess.call(args, stdin=fin,
                    stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            self._check_rc(cp)
        finally:
            if fin:
                fin.close()

        cp = subprocess.run(["perf", "report", "--sort=dso", "--stdio"],
            stdout=subprocess.PIPE, universal_newlines=True)
        cp.check_returncode()

        patt = re.compile("([0-9]+\.[0-9]+)% +(.*)")
        p = None
        for l in cp.stdout.split('\n'):
            l = l.strip()
            if not l or l[0] == '#':
                continue
            if verbose:
                print(l)
            m = patt.match(l)
            if m and m.group(2) == self.name:
                p = float(m.group(1))
                p = p / 100

        if p:
            if verbose:
                print("{0}: {1:.3f}".format(self.name, p))
            self.lcperfs.append(p)
        else:
            raise Exception("Failed to perf libc!")


    def run(self, i):
        fin = None
        fout = None

        try:
            stdin = self.opts.stdin
            if stdin:
                fin = open(stdin, 'rb')
            fout = open(self._out(i), 'wb')

            perf_out = self._perf_out(i)
            args = ["perf", "stat", "-o", perf_out] + self.args

            # t0 = time.time()
            cp = subprocess.call(args, stdin=fin, stdout=fout,
                    stderr=subprocess.DEVNULL)
            # t1 = time.time()
            self._check_rc(cp)
            # t = t1 - t0
            t = self._extract_perf_time(perf_out)

        finally:
            if fin:
                fin.close()
            if fout:
                fout.close()

        self.times.append(t)
        if self.opts.verbose:
            print("run #" + str(i) + ": time taken:", t)
        sys.stdout.flush()


class Measure:
    def __init__(self, dir, prog, args, opts):
        self.dir = dir
        self.prog = prog
        self.args = args
        self.opts = opts


    def measure(self):
        for target in ['x86']:
            self._measure_target(target)


    def perf(self):
        for target in ['x86']:
            self._perf_target(target)


    def _build_programs(self, target):
        nprog = Program(self.dir, self.prog, target, None, self.args, self.opts)
        xarch = 'rv32-' + target
        xprogs = [
            Program(self.dir, self.prog, xarch, mode, self.args, self.opts)
                for mode in SBT.modes]
        return [nprog] + xprogs


    class Result:
        # m: mean
        # sd: standard deviation
        # t: time
        # p: perf/percentage of total time spent on main bench code
        # f: final

        def __init__(self, times, lcperfs):
            # functions' aliases
            mean = Measure.mean
            sd = Measure.sd
            sqr = self.sqr
            sqrt = math.sqrt

            # time
            tm = mean(times)
            tsd = sd(times, tm)

            # %
            pm = mean(lcperfs)
            psd = sd(lcperfs, pm)

            # final
            fm = tm * pm
            # sqrt(...) -> error propagation
            fsd = fm * sqrt(sqr(tsd/tm) + sqr(psd/pm))

            # save results in self
            self.tm = tm
            self.tsd = tsd
            self.pm = pm
            self.psd = psd
            self.fm = fm
            self.fsd = fsd


        @staticmethod
        def sqr(x):
            return x*x


        @staticmethod
        def slowdown(res, nat):
            if res == nat:
                return (1, 0)

            # time based only
            time_based_only = False
            if time_based_only:
                return (1 + (res.tm - nat.tm) / nat.tm, 0)

            sqr = Measure.Result.sqr
            sqrt = math.sqrt

            m = res.fm / nat.fm
            sd = m * sqrt(sqr(res.fsd/res.fm) + sqr(nat.fsd/nat.fm))
            return (m, sd)


    def _measure_target(self, target):
        progs = self._build_programs(target)

        # run
        N = 10
        times = {}
        lcperfs = {}
        # stringsearch runs to fast, so we need to increase the
        # sample frequency in its case
        freq = 20000 if self.prog == "stringsearch" else None
        for prog in progs:
            if self.opts.verbose:
                print("measuring", prog.name)
            for i in range(N):
                prog.perf_libc(freq=freq)
            for i in range(N):
                prog.run(i)
            times[prog.mode] = prog.times
            lcperfs[prog.mode] = prog.lcperfs

        # get means
        nat = None
        Result = Measure.Result

        # mode len
        ml = 8
        # precisions
        p = [
                # times
                8, 5, 8, 5,
                # %'s
                5, 2, 5, 2,
                # final times
                8, 5, 8, 5,
                # slowdown
                5, 2, 5, 2]
        # caption lengths
        l = [p[i] for i in range(0, len(p), 2)]

        # format strings
        header = ("{{:<{}}} " + " {{:<{}}}"  * 8 + " ({{}})").format(ml, *l)
        row =    ("{{:<{}}}:" + " {{: <{}.{}f}}" * 8).format(ml, *p)

        print(header.format("mode",
            "tmean", "tsd", "%mean", "%sd", "mean", "sd",
            "x", "xsd", self.prog))
        for mode in ['native'] + SBT.modes:
            res = Result(times[mode], lcperfs[mode])
            if mode == 'native':
                nat = res

            (x, xsd) = Result.slowdown(res, nat)
            print(row.format(mode,
                res.tm, res.tsd, res.pm * 100, res.psd * 100, res.fm, res.fsd,
                x, xsd))


    def _perf_target(self, target):
        progs = self._build_programs(target)

        for prog in progs:
            prog.perf_libc(verbose=True)


    @staticmethod
    def mean(vals):
        return sum(vals) / len(vals)


    @staticmethod
    def sd(vals, mean):
        return math.sqrt(
            sum([math.pow(val - mean, 2) for val in vals])
            / (len(vals) - 1))


    @staticmethod
    def median(vals):
        vals.sort()
        n = len(vals)
        if n % 2 == 0:
            v1 = vals[n//2 -1]
            v2 = vals[n//2]
            return (v1 + v2) / 2
        else:
            return vals[n//2 +1]


    @staticmethod
    def geomean(iterable):
        a = np.array(iterable)
        return a.prod()**(1.0/len(a))



# dir test arg
if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Measure translation overhead')
    parser.add_argument('dir', type=str)
    parser.add_argument('test', type=str)
    parser.add_argument("--args", nargs="*", metavar="arg", default=[])
    parser.add_argument("--stdin", help="stdin redirection")
    parser.add_argument("-v", action="store_true", help="verbose")
    parser.add_argument("--exp-rc", type=int, default=0,
        help="expected return code")
    parser.add_argument("--perf", action="store_true")

    args = parser.parse_args()
    sargs = [arg.strip() for arg in args.args]

    # args.v = True
    opts = Options(args.stdin, args.v, args.exp_rc)
    measure = Measure(args.dir, args.test, sargs, opts)

    if args.perf:
        measure.perf()
    else:
        measure.measure()

