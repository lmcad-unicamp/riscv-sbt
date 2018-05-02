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
        self.freq = None
        self.perf_libc = True
        self.perf = True
        self.csv = True
        self.id = None
        self.save_out = False


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


    def _check_rc(self, rc, args=None):
        exp_rc = self.opts.exp_rc
        if rc != exp_rc:
            raise Exception("Failure! rc=" + str(rc) + " exp_rc=" + str(exp_rc)
                    + ("\nCommand: " + " ".join(args) if args else ""))


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
        save_out = self.opts.save_out

        args = ["perf", "record", "-q"]
        if freq:
            args.extend(["-F", str(freq)])
        elif self.opts.freq:
            args.extend(["-F", str(self.opts.freq)])
        args.extend(self.args)
        if verbose:
            print(" ".join(args))

        fin = None
        fout = None
        try:
            stdin = self.opts.stdin
            if stdin:
                fin = open(stdin, 'rb')
            if save_out:
                fout = open(self._out(0), 'wb')
            else:
                fout = subprocess.DEVNULL

            cp = subprocess.call(args, stdin=fin, stdout=fout,
                    stderr=subprocess.DEVNULL)
            self._check_rc(cp, args)
        finally:
            if fin:
                fin.close()
            if save_out and fout:
                fout.close()

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
        save_out = self.opts.save_out

        fin = None
        fout = None

        try:
            stdin = self.opts.stdin
            if stdin:
                fin = open(stdin, 'rb')
            if save_out:
                fout = open(self._out(i), 'wb')
            else:
                fout = subprocess.DEVNULL

            perf_out = self._perf_out(i)
            perf = self.opts.perf
            if perf:
                args = ["perf", "stat", "-o", perf_out]
            else:
                args = []
            args.extend(self.args)

            if not perf:
                t0 = time.time()
            cp = subprocess.call(args, stdin=fin, stdout=fout,
                    stderr=subprocess.DEVNULL)
            if not perf:
                t1 = time.time()
                t = t1 - t0
            self._check_rc(cp, args)
            if perf:
                t = self._extract_perf_time(perf_out)

        finally:
            if fin:
                fin.close()
            if save_out and fout:
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
        if opts.id:
            self.id = opts.id
        else:
            self.id = prog


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
        # mode len
        ml = 8

        # m: mean
        # sd: standard deviation
        # t: time
        # p: perf/percentage of total time spent on main bench code
        # f: final

        class Field:
            def __init__(self, m, sd, p):
                self.m = m
                self.sd = sd
                self.p = p


            def _to_str(self, v, align):
                fmt = ("{{: <{}.{}f}}").format(
                        self.p[0] if align else self.p[1], self.p[1])
                return fmt.format(v)


            def to_str_list(self, align=True):
                return [ self._to_str(self.m, align),
                         self._to_str(self.sd, align) ]


        def __init__(self, mode, times, lcperfs):
            # functions' aliases
            mean = Measure.mean
            sd = Measure.sd
            sqr = self.sqr
            sqrt = math.sqrt

            # time
            tm = mean(times)
            tsd = sd(times, tm)

            # %
            if len(lcperfs):
                pm = mean(lcperfs)
                psd = sd(lcperfs, pm)
            else:
                pm = 1
                psd = 0

            # final
            fm = tm * pm
            # sqrt(...) -> error propagation
            fsd = fm * sqrt(sqr(tsd/tm) + sqr(psd/pm))

            # save results in self
            self.mode = mode
            Field = self.Field
            self.t = Field(tm, tsd, [8, 5])
            self.p = Field(pm, psd, [5, 2])
            self.f = Field(fm, fsd, [8, 5])
            self.x = Field(0, 0, [5, 2])


        @staticmethod
        def sqr(x):
            return x*x


        def slowdown(self, nat):
            if self == nat:
                self.x.m = 1
                self.x.sd = 0
                return

            sqr = self.sqr
            sqrt = math.sqrt

            m = self.f.m / nat.f.m
            sd = m * sqrt(sqr(self.f.sd/self.f.m) + sqr(nat.f.sd/nat.f.m))
            self.x.m = m
            self.x.sd = sd


        def to_str_list(self, align=True):
            p100 = self.p
            p100.m = p100.m * 100
            p100.sd = p100.sd * 100

            l = (
                self.t.to_str_list(align) +
                p100.to_str_list(align) +
                self.f.to_str_list(align) +
                self.x.to_str_list(align))

            if align:
                mode_str = ("{:<" + str(self.ml) + "}").format(self.mode)
            else:
                mode_str = self.mode

            return [mode_str] + l


        def print(self):
            print(" ".join(self.to_str_list()))


    def _measure_target(self, target):
        progs = self._build_programs(target)

        # run
        N = 10
        times = {}
        lcperfs = {}
        for prog in progs:
            if self.opts.verbose:
                print("measuring", prog.name)
            if self.opts.perf_libc:
                for i in range(N):
                    prog.perf_libc()
            for i in range(N):
                prog.run(i)
            times[prog.mode] = prog.times
            lcperfs[prog.mode] = prog.lcperfs

        # get means
        nat = None
        Result = Measure.Result

        # precisions
        r = Result("", [1, 2], [])
        l0 = [
            r.t.p[0],
            r.p.p[0],
            r.f.p[0],
            r.x.p[0]]
        l = [i  for i in l0
                for j in range(2)]

        # format strings
        header = ("{{:<{}}} " + "{{:<{}}} " * 8 + " ({{}})").format(
                Result.ml, *l)

        print(header.format("mode",
            "tmean", "tsd", "%mean", "%sd", "mean", "sd",
            "x", "xsd", self.id))
        results = []
        for mode in ['native'] + SBT.modes:
            res = Result(mode, times[mode], lcperfs[mode])
            if mode == 'native':
                nat = res

            res.slowdown(nat)
            res.print()
            results.append(res)

        if self.opts.csv:
            self._csv_append(results)


    def _csv_append(self, results):
        dir = os.path.split(self.dir)[0]
        if self.prog.startswith("adpcm"):
            dir = os.path.split(dir)[0]
            dir = os.path.split(dir)[1]
        else:
            dir = os.path.split(dir)[1]


        align = False
        row = [self.id, dir]
        for res in results:
            l = (res.f.to_str_list(align) +
                 res.x.to_str_list(align))
            row.extend(l)

        with open("mibench.csv", "a") as f:
            f.write(",".join(row) + "\n")


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
    parser.add_argument("--freq", type=int,
        help="perf sample frequency")
    parser.add_argument("--no-perf-libc", action="store_true")
    parser.add_argument("--no-perf", action="store_true",
        help="don't use perf")
    parser.add_argument("--no-csv", action="store_true",
        help="don't append output to .csv file")
    parser.add_argument("--id", type=str)

    args = parser.parse_args()
    sargs = [arg.strip() for arg in args.args]
    if args.no_perf:
        args.no_perf_libc = True

    # args.v = True
    opts = Options(args.stdin, args.v, args.exp_rc)
    opts.freq = args.freq
    opts.perf_libc = not args.no_perf_libc
    opts.perf = not args.no_perf
    opts.csv = not args.no_csv
    opts.id = args.id
    measure = Measure(args.dir, args.test, sargs, opts)

    if args.perf:
        measure.perf()
    else:
        measure.measure()

