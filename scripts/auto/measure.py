#!/usr/bin/python3

from auto.config import ARM, GCC7, RV32_LINUX, SBT, X86
from auto.utils import path, shell

import argparse
import math
import numpy as np
import os
import re
import subprocess
import sys
import time

PERF = os.getenv("PERF", "perf")
RV32_MODES = ["native", "QEMU"]
SBT_MODES = ["native"] + SBT.modes
ALL_MODES = RV32_MODES + SBT.modes
ALL_COLUMNS = [i for i in range(8)]

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
        self.modes = None
        self.rv32 = None


class Program:
    def __init__(self, dir, basename, arch, mode, args, opts):
        self.basename = basename
        self.name = arch + '-' + basename
        self.rv32 = arch == "rv32"
        if mode and mode == "QEMU":
            pass
        elif mode:
            self.name = self.name + '-' + mode
        else:
            mode = 'native'
        self.mode = mode
        self.args = [path(dir, self.name)] + args
        self.oargs = list(self.args)
        self.opts = opts
        self.times = []
        self.lcperfs = []

        self.ins = []
        self.outs = []
        if self.basename in ["dijkstra", "crc32", "sha", "patricia"]:
            self.ins = [1]
        elif self.basename in ["rijndael", "susan", "lame"]:
            self.ins = [1]
            self.outs = [2]
        elif self.basename == "blowfish":
            self.ins = [2]
            self.outs = [3]
        self.stdin = opts.stdin

        if self.opts.verbose:
            print(" ".join(self.args))
        self._copy_ins()
        if self.opts.verbose:
            print(" ".join(self.args))


    def _to_tmp(self, i, copy):
        arg = self.args[i]
        out = path("/tmp", os.path.basename(arg))
        if copy:
            shell("cp {} {}".format(arg, out))
        self.args[i] = out


    def _copy_ins(self):
        for i in self.ins:
            self._to_tmp(i, copy=True)
        for o in self.outs:
            self._to_tmp(o, copy=False)

        if self.stdin:
            arg = self.stdin
            out = path("/tmp", os.path.basename(arg))
            shell("cp {} {}".format(arg, out))
            self.stdin = out


    def copy_outs(self):
        for o in self.outs:
            arg = self.args[o]
            oarg = self.oargs[o]
            shell("cp {} {}".format(arg, oarg))


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

        if self.rv32:
            args.extend(["qemu-riscv32", "-L", RV32_LINUX.sysroot])

        args.extend(self.args)
        if verbose:
            print(" ".join(args))

        fin = None
        fout = None
        try:
            stdin = self.stdin
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
            stdin = self.stdin
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

            if self.rv32:
                args.extend(["qemu-riscv32", "-L", RV32_LINUX.sysroot])

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


class MiBench:
    def __init__(self, modes=None, columns=None):
        self.modes = modes if modes else ALL_MODES
        self.columns = columns if columns else ALL_COLUMNS
        # precisions
        p = [p[0] for p in Result.precisions()
                  for j in range(2)]
        self.p = [p[i] for i in self.columns]


    def header(self):
        hdr = []

        ispace = ["" for i in range(self.item_len()-1)]
        l = ["benchmarks"]
        for mode in self.modes:
            l.extend([mode] + ispace)
        hdr.append(l)

        tm = "tmean"
        tsd = "tsd"
        pm = "%mean"
        psd = "%sd"
        fm = "mean"
        fsd = "sd"
        x = "x"
        xsd = "xsd"
        tmp = [tm, tsd, pm, psd, fm, fsd, x, xsd]
        item = [tmp[i] for i in self.columns]

        l = ["name"]
        l.extend(item * len(self.modes))
        hdr.append(l)

        return hdr


    def item_len(self):
        return len(self.columns)


    @staticmethod
    def item_max_len():
        return len(ALL_COLUMNS)


    def item_format(self, *args):
        argi = [*args]
        argo = [argi[i] for i in self.columns]
        # format strings
        fmt = ("{{:<{}}} " * (self.item_len()-1) + "{{:<{}}}").format(
                *self.p)
        return fmt.format(*argo)


    def line_format(self, *args):
        # name
        argi = [*args]
        out = "{{:<{}}}".format(Results.F1_ALIGN).format(argi[0])

        idx = 1
        for mode in ALL_MODES:
            if mode in self.modes:
                out = (out + (" " if idx > 1 else "") +
                    self.item_format(*argi[idx:idx+self.item_max_len()]))
            idx = idx + self.item_max_len()

        return out


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
        res = Result(mode, None, None, noinit=True)
        Field = Result.Field
        res.t = Field(float(parts[0]), float(parts[1]), res.tp)
        res.p = Field(float(parts[2])/100, float(parts[3])/100, res.pp)
        res.f = Field(float(parts[4]), float(parts[5]), res.fp)
        res.x = Field(float(parts[6]), float(parts[7]), res.xp)
        return res


    @staticmethod
    def get_neutral(mode="neutral"):
        res = Result(mode, None, None, noinit=True)
        Field = Result.Field
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
        for mode in ALL_MODES:
            res = Result.build(mode, parts[idx:idx+8])
            idx = idx + 8
            results.append(res)
        return Results(name, results)


    def to_str_list(self):
        sl = [self.name]
        for res in self.results:
            l = res.to_str_list(align=False, mode=False)
            sl.extend(l)
        return sl


    def print(self, modes, cols):
        mibench = MiBench(modes, cols)
        print(mibench.line_format(*self.to_str_list()))


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
        acc = Results("geomean", [Result.get_neutral(mode)
                for mode in ALL_MODES])

        for res in results:
            acc.mul(res)
        acc.xrt(len(results))
        return acc


class CSV:
    def __init__(self, headers, results):
        self.headers = headers
        self.results = results
        self.lines = None


    @staticmethod
    def read(csv):
        headers = []
        results = []
        with open(csv, "r") as f:
            for ln in f:
                ln = ln.strip()
                if len(headers) < 2:
                    headers.append(ln.split(","))
                    continue
                res = Results.parse(ln)
                results.append(res)
        return CSV(headers, results)


    def _build_lines(self, modes=None, cols=None):
        if self.lines != None:
            return

        lines = []
        for h in self.headers:
            lines.append(h)
        for res in self.results:
            lines.append(self._xform_res(res, modes, cols))
        self.lines = lines


    def print(self):
        self._build_lines()
        for ln in self.lines:
            print(",".join(ln))


    def _print_header(self, header, parts, modes, cols):
        mibench = MiBench(modes, cols)
        if header == 0:
            out = "{{:<{}}}".format(Results.F1_ALIGN).format(parts[0])
            idx = 1
            n = mibench.item_max_len()
            p = sum(mibench.p) + len(mibench.p) - 1
            for mode in ALL_MODES:
                if mode in opts.modes:
                    out = (out + (" " if idx > 1 else "") +
                            ("{:<" + str(p) + "}").format(parts[idx]))
                idx = idx + n
            print(out)
            return
        print(mibench.line_format(*parts))


    def _print_headers(self, modes, cols):
        for i in range(len(self.headers)):
            self._print_header(i, self.headers[i], modes, cols)


    def printf(self, modes, cols):
        self._print_headers(modes, cols)
        for res in self.results:
            res.print(modes, cols)
        Results.geomean(self.results).print(modes, cols)


    def _xform(self, parts, func):
        idx = 1
        n = MiBench.item_max_len()
        nparts = [parts[0]]
        return func(parts, nparts, idx, n)


    def _xform_h0(self, modes, cols):
        def xform_func(parts, nparts, idx, n):
            for mode in ALL_MODES:
                if mode in modes:
                    nparts.append(parts[idx])
                    for i in range(len(cols)-1):
                        nparts.append("")
                idx = idx + n
            return nparts
        return xform_func


    def _xform_modes_cols(self, modes, cols):
        def xform_func(parts, nparts, idx, n):
            for mode in ALL_MODES:
                if mode in modes:
                    for i in cols:
                        nparts.append(parts[idx+i])
                idx = idx + n
            return nparts
        return xform_func


    def _xform_header(self, header, parts, modes, cols):
        if header == 0:
            func = self._xform_h0(modes, cols)
        else:
            func = self._xform_modes_cols(modes, cols)
        self.headers[header] = self._xform(parts, func)


    def _xform_headers(self, modes, cols):
        for i in range(len(self.headers)):
            self._xform_header(i, self.headers[i], modes, cols)


    def _xform_res(self, res, modes, cols):
        sl = res.to_str_list()
        if modes == None and cols == None:
            return sl

        func = self._xform_modes_cols(modes, cols)
        return self._xform(sl, func)


    def append_geomean(self):
        self.results.append(Results.geomean(self.results))


    def xform(self, modes, cols):
        self._xform_headers(modes, cols)
        self._build_lines(modes, cols)
        self.print()


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
        self._measure_target(self.narch.prefix)


    def perf(self):
        self._perf_target(self.narch.prefix)


    def _build_programs(self, target):
        nprog = Program(self.dir, self.prog, target, None, self.args, self.opts)

        if self.opts.rv32:
            xarch = "rv32"
            xprogs = [ Program(self.dir, self.prog, xarch, "QEMU", self.args,
                    self.opts) ]
        else:
            xarch = 'rv32-' + target
            xprogs = [
                Program(self.dir, self.prog, xarch, mode, self.args, self.opts)
                    for mode in self.opts.modes if mode != "native"]
        return [nprog] + xprogs


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
            prog.copy_outs()
            times[prog.mode] = prog.times
            lcperfs[prog.mode] = prog.lcperfs

        # get means
        nat = None

        if self.opts.rv32:
            modes = ["native", "QEMU"]
            mibench = MiBench(modes, ALL_COLUMNS)
        else:
            mibench = MiBench(ALL_MODES, ALL_COLUMNS)
            modes = self.opts.modes

        print(
            "{{:<{}}} ".format(Result.MODE_LEN).format("mode"),
            mibench.item_format(*mibench.header()[1][1:9]),
            " ({})".format(self.id), sep='')

        results = []
        for mode in modes:
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
    parser.add_argument("--printf", "-p", action="store_true",
        help="print formatted .csv file contents")
    parser.add_argument("--modes", "-m", type=str, nargs='+',
        choices=ALL_MODES, default=SBT_MODES)
    parser.add_argument("--columns", "-c", type=int, nargs='+',
        choices=ALL_COLUMNS, default=ALL_COLUMNS)
    parser.add_argument("--xform", "-x", action="store_true",
        help="apply modes/columns filters and output new .csv file")
    parser.add_argument("--rv32", action="store_true",
        help="measure RV32 emulator (QEMU) performance")

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
    n = os.getenv("N")
    opts.n = int(n) if n else args.n
    opts.modes = args.modes
    opts.columns = args.columns

    opts.rv32 = args.rv32
    if opts.rv32:
        opts.perf_libc = False
        ALL_MODES = RV32_MODES
    else:
        ALL_MODES = SBT_MODES

    pargs = args.pargs

    if args.printf or args.xform:
        if len(pargs) != 1:
            raise Exception("Wrong number of arguments")
        csv = CSV.read(pargs[0])
        if args.printf:
            csv.printf(opts.modes, opts.columns)
        else:
            csv.append_geomean()
            csv.xform(opts.modes, opts.columns)
    else:
        if len(pargs) != 2:
            raise Exception("Wrong number of arguments")
        measure = Measure(pargs[0], pargs[1], sargs, opts)
        if args.perf:
            measure.perf()
        else:
            measure.measure()

