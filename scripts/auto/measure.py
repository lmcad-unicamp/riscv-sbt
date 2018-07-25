#!/usr/bin/python3

from auto.config import ARM, GCC7, SBT, X86
from auto.utils import path, shell

import argparse
import math
import numpy as np
import os
import re
import subprocess
import sys
import time

PERF = "perf_4.16" if GCC7 else "perf_4.9"

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
        self.n = None


class Program:
    def __init__(self, dir, basename, arch, mode, args, opts):
        self.basename = basename
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
        self._copy_files()
        if self.opts.verbose:
            print(" ".join(self.args))


    def _copy_files(self):
        if self.basename == "rijndael":
            # encode
            ins = [1]
            outs = [2]
            # TODO decode
        else:
            ins = []
            outs = []

        def to_tmp(i, copy):
            arg = self.args[i]
            out = path("/tmp", os.path.basename(arg))
            if copy:
                shell("cp {} {}".format(arg, out))
            self.args[i] = out

        for i in ins:
            to_tmp(i, copy=True)
        for o in outs:
            to_tmp(o, copy=False)


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

        args = [PERF, "record", "-q"]
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

        cp = subprocess.run([PERF, "report", "--sort=dso", "--stdio"],
            stdout=subprocess.PIPE, universal_newlines=True)
        cp.check_returncode()
        shell("cp perf.data perf-" + self.name + ".data", quiet=True)

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
                args = [PERF, "stat", "-o", perf_out]
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
        if X86.is_native():
            self.narch = X86
        elif ARM.is_native():
            self.narch = ARM
        else:
            raise Exception("Unsupported native target")


    def measure(self):
        self._measure_target(self.narch.name)


    def perf(self):
        self._perf_target(self.narch.name)


    def _build_programs(self, target):
        nprog = Program(self.dir, self.prog, target, None, self.args, self.opts)
        xarch = 'rv32-' + target
        xprogs = [
            Program(self.dir, self.prog, xarch, mode, self.args, self.opts)
                for mode in SBT.modes]
        return [nprog] + xprogs


    class Result:
        MODE_LEN = 8

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


            def mul(self, f):
                self.m = self.m * f.m
                self.sd = self.sd * f.sd


            def xrt(self, n):
                self.m = self.m**(1.0/n)
                self.sd = self.sd**(1.0/n)


        @staticmethod
        def precisions():
            return [
                [7, 4],
                [5, 2],
                [7, 4],
                [5, 2]]


        def _set_precisions(self):
            p = self.precisions()
            self.tp = p[0]
            self.pp = p[1]
            self.fp = p[2]
            self.xp = p[3]


        def __init__(self, mode, times, lcperfs, noinit=False):
            self.mode = mode
            self._set_precisions()
            if noinit:
                return

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
            Field = self.Field
            self.t = Field(tm, tsd, self.tp)
            self.p = Field(pm, psd, self.pp)
            self.f = Field(fm, fsd, self.fp)
            self.x = Field(0, 0, self.xp)


        def mul(self, res):
            self.t.mul(res.t)
            self.p.mul(res.p)
            self.f.mul(res.f)
            self.x.mul(res.x)


        def xrt(self, n):
            self.t.xrt(n)
            self.p.xrt(n)
            self.f.xrt(n)
            self.x.xrt(n)


        @staticmethod
        def build(mode, parts):
            res = Measure.Result(mode, None, None, noinit=True)
            Field = Measure.Result.Field
            res.t = Field(float(parts[0]), float(parts[1]), res.tp)
            res.p = Field(float(parts[2])/100, float(parts[3])/100, res.pp)
            res.f = Field(float(parts[4]), float(parts[5]), res.fp)
            res.x = Field(float(parts[6]), float(parts[7]), res.xp)
            return res


        @staticmethod
        def get_neutral(mode="neutral"):
            res = Measure.Result(mode, None, None, noinit=True)
            Field = Measure.Result.Field
            res.t = Field(1.00, 1.00, res.tp)
            res.p = Field(1.00, 1.00, res.pp)
            res.f = Field(1.00, 1.00, res.fp)
            res.x = Field(1.00, 1.00, res.xp)
            return res


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


        def to_str_list(self, align=True, mode=True, adjust_p=True):
            if adjust_p:
                p = self.p
                p100 = self.Field(p.m, p.sd, p.p)
                p100.m = p100.m * 100
                p100.sd = p100.sd * 100
            else:
                p100 = self.p

            l = (
                self.t.to_str_list(align) +
                (p100.to_str_list(align) if p100 else []) +
                (self.f.to_str_list(align) if self.f else []) +
                self.x.to_str_list(align))

            if not mode:
                return l

            if align:
                mode_str = ("{:<" + str(self.MODE_LEN) + "}").format(self.mode)
            else:
                mode_str = self.mode

            return [mode_str] + l


        def print(self):
            print(" ".join(self.to_str_list()))


    class Results:
        F1_ALIGN = "14"

        def __init__(self, name, results):
            self.name = name
            self.set = set
            self.results = results


        @staticmethod
        def parse(ln):
            parts = ln.split(',')
            idx = 0
            name = parts[idx]
            name = name.replace("encode", "enc")
            name = name.replace("decode", "dec")
            name = name.replace("smoothing", "smooth")
            idx = idx + 1

            results = []
            for mode in ['native'] + SBT.modes:
                res = Measure.Result.build(mode, parts[idx:idx+8])
                idx = idx + 8
                results.append(res)
            return Measure.Results(name, results)


        def print(self):
            print(("{0:<" + self.F1_ALIGN + "}").format(self.name), end='')
            for res in self.results:
                sl = res.to_str_list(mode=False)
                print(" ".join(sl), end=' ')
            print()


        def mul(self, results):
            for i in range(len(self.results)):
                r0 = self.results[i]
                r1 = results.results[i]
                r0.mul(r1)


        def xrt(self, n):
            for res in self.results:
                res.xrt(n)


        @staticmethod
        def geomean(results):
            Result = Measure.Result
            Results = Measure.Results

            acc = Results("geomean", [Result.get_neutral(mode)
                    for mode in ['native'] + SBT.modes])

            for res in results:
                acc.mul(res)
            acc.xrt(len(results))
            return acc


    class MiBench:
        @staticmethod
        def header():
            hdr = []

            ispace = ["" for i in range(7)]
            l = ["benchmarks"]
            l.extend(["native"] + ispace)
            l.extend(["globals"] + ispace)
            l.extend(["locals"] + ispace)
            hdr.append(l)

            tm = "tmean"
            tsd = "tsd"
            pm = "%mean"
            psd = "%sd"
            fm = "mean"
            fsd = "sd"
            x = "x"
            xsd = "xsd"
            item = [tm, tsd, pm, psd, fm, fsd, x, xsd]

            l = ["name"]
            l.extend(item * 3)
            hdr.append(l)

            return hdr


        @staticmethod
        def item_format_str():
            # precisions
            p = [p[0] for p in Measure.Result.precisions()
                      for j in range(2)]
            # format strings
            fmt = ("{{:<{}}} " * 7 + "{{:<{}}}").format(*p)
            return fmt


        @staticmethod
        def item_format(*args):
            self = Measure.MiBench
            return self.item_format_str().format(*args)


        @staticmethod
        def line_format_str():
            self = Measure.MiBench
            ifmt = self.item_format_str()
            fhdr = ("{{:<{}}}".format(Measure.Results.F1_ALIGN) +
                    (ifmt + " ") * 2 + ifmt)
            return fhdr


        @staticmethod
        def line_format(*args):
            self = Measure.MiBench
            return self.line_format_str().format(*args)


    def _measure_target(self, target):
        progs = self._build_programs(target)

        # run
        N = self.opts.n
        times = {}
        lcperfs = {}
        verbose = self.opts.verbose
        for prog in progs:
            if verbose:
                print("measuring", prog.name)
            if self.opts.perf_libc:
                for i in range(N):
                    prog.perf_libc(verbose=verbose)
                    print('c', end='')
            for i in range(N):
                prog.run(i)
                print('*', end='')
            print()
            times[prog.mode] = prog.times
            lcperfs[prog.mode] = prog.lcperfs

        # get means
        nat = None
        Result = Measure.Result
        MiBench = Measure.MiBench

        print(
            "{{:<{}}} ".format(Measure.Result.MODE_LEN).format("mode"),
            MiBench.item_format(*MiBench.header()[1][1:9]),
            " ({})".format(self.id), sep='')

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
        align = False
        row = [self.id]
        for res in results:
            l = res.to_str_list(align, mode=False)
            row.extend(l)

        with open("mibench.csv", "a") as f:
            f.write(",".join(row) + "\n")


    def _print_header(self, header, ln):
        MiBench = Measure.MiBench
        parts = ln.split(',')
        print(MiBench.line_format(*parts))


    def format(self, csv):
        headers = 0
        i = 0
        n = 30
        results = []
        with open(csv, "r") as f:
            for ln in f:
                ln = ln.strip()
                if headers < 2:
                    self._print_header(headers, ln)
                    headers = headers + 1
                    continue

                res = self.Results.parse(ln)
                res.print()
                results.append(res)
                i = i + 1
                if i == n:
                    break
        Measure.Results.geomean(results).print()


    def _perf_target(self, target):
        progs = self._build_programs(target)

        for prog in progs:
            prog.perf_libc(verbose=True)


    @staticmethod
    def mean(vals):
        return sum(vals) / len(vals)


    @staticmethod
    def sd(vals, mean):
        if len(vals) == 1:
            return 0
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
    parser.add_argument('pargs', type=str, nargs="*", metavar="parg")
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
    parser.add_argument("-n", type=int, default=10,
        help="number of runs")
    parser.add_argument("--format", "-f", action="store_true",
        help="show .csv file contents as formatted entries")

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
    opts.n = args.n

    pargs = args.pargs

    if args.format:
        if len(pargs) != 1:
            raise Exception("Wrong number of arguments")
        measure = Measure(None, None, sargs, opts)
        measure.format(pargs[0])
    else:
        if len(pargs) != 2:
            raise Exception("Wrong number of arguments")
        measure = Measure(pargs[0], pargs[1], sargs, opts)
        if args.perf:
            measure.perf()
        else:
            measure.measure()

