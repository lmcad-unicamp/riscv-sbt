#!/usr/bin/env python3

from auto.config import ARM, GOPTS, SBT, TOOLS, X86
from auto.utils import cat, path, unique

class ArchAndMode:
    def __init__(self, farch, narch, mode=None):
        self.farch = farch
        self.narch = narch
        self.mode = mode

    # get arch prefix string
    def prefix(self):
        if not self.farch:
            return self.narch.prefix
        elif not self.narch:
            return self.farch.prefix
        else:
            return self.farch.add_prefix(self.narch.prefix)

    # get output file name for this ArchAndMode
    def bin(self, name):
        if not self.prefix():
            raise Exception("No prefix")
        if not name:
            raise Exception("No name")

        return (self.prefix() + "-" + name +
            ("-" + self.mode if self.mode else ""));


    @staticmethod
    def sfmt(templ, prefix, mode):
        return templ.format(**{
            "prefix":   prefix,
            "mode":     mode,
            })


    def fmt(self, templ):
        prefix = self.prefix()
        if prefix:
            prefix = prefix + '-'
        else:
            prefix = ''

        mode = self.mode
        if mode:
            mode = '-' + mode
        else:
            mode = ''

        return self.sfmt(templ, prefix, mode)


# arguments for a given run
class Run:
    def __init__(self, args, id=None, rflags=None, outidx=None, stdin=None):
        self.args = args
        self.rflags_arg = rflags
        self.id = id
        self.outidx = outidx
        self.stdin = stdin


    @staticmethod
    def out_suffix():
        return ".out"


    @staticmethod
    def build_name(am, base, id, suffix):
        if am:
            r = am.bin(base)
        else:
            r = base
        if id:
            r = r + '-' + id
        if suffix:
            r = r + suffix
        return r


    def bin(self, am, name):
        return am.bin(name)


    def out(self, am, name):
        if self.outidx == None:
            if am:
                return self.build_name(am, name, self.id, self.out_suffix())
            else:
                return None
        return am.fmt(self.args[self.outidx])


    def build(self, am):
        if am:
            return [am.fmt(arg) for arg in self.args]
        return [ArchAndMode.sfmt(arg, '', '') for arg in self.args]


    @staticmethod
    def _escape(args):
        args2 = []
        for arg in args:
            if len(arg) > 0 and arg[0] == '-':
                arg = '" {}"'.format(arg)
            args2.append(arg)
        return args2


    def str(self, am):
        args = self.build(am)
        if not args:
            return ''

        args2 = ["--args"]
        args2.extend(self._escape(args))
        return " ".join(args2)


    def rflags(self, am, name):
        args_str = self.str(am)
        out = self.out(am, name)
        if not self.outidx:
            args_str = cat('-o', out, args_str)

        if self.stdin:
            args_str = cat(args_str, "<", self.stdin)

        return cat(self.rflags_arg, args_str)


class Runs:
    def __init__(self, runs=[], name=None):
        self.runs = runs
        self.name = name

    def add(self, run):
        self.runs.append(run)

    def __iter__(self):
        return self.runs.__iter__()


class GenMake:
    def __init__(self, narchs, xarchs,
            srcdir, dstdir, name,
            xflags, bflags, mflags, sbtflags=[]):
        self.narchs = narchs
        self.xarchs = xarchs
        self.srcdir = srcdir
        self.dstdir = dstdir
        self.name = name
        self.xflags = xflags
        self.bflags = bflags
        self.mflags = mflags
        self.sbtflags = sbtflags
        #
        self.out_filter = None
        #
        self.txt = "### {} ###\n\n".format(name)


    def append(self, txt):
        self.txt = self.txt + txt


    def bld(self, arch, ins, out):
        out_is_obj = out.endswith(".o")

        objs = []
        aobjs = []
        if len(ins) == 1:
            if not out_is_obj:
                objs = [arch.out2objname(out)]
                aobjs = [path(self.dstdir, objs[0])]
        else:
            for src in ins:
                obj = arch.src2objname(src)
                if not out_is_obj or obj != out:
                    objs.append(obj)
            aobjs = [path(self.dstdir, obj) for obj in objs]

        ains = [path(self.srcdir, i) for i in ins]

        fmtdata = {
            "arch":     arch.name,
            "aobjs":    " ".join(aobjs),
            "srcdir":   self.srcdir,
            "dstdir":   self.dstdir,
            "ins":      " ".join(ins),
            "ains":     " ".join(ains),
            "out":      out,
            "bflags":   " " + self.bflags if self.bflags else "",
            "build":    TOOLS.build,
        }

        self.append("""\
.PHONY: {out}
{out}: {dstdir}/{out}

{dstdir}/{out} {aobjs}: {ains}
\t{build} --arch {arch} --srcdir {srcdir} --dstdir {dstdir} {ins} -o {out}{bflags}

""".format(**fmtdata))


    def _ssh_copy(self, fmtdata):
        self.append("""\
.PHONY: {tgt}
{tgt}: {out}
\tscp {src} {rem}:{dst}

""".format(**fmtdata))


    def _adb_copy(self, fmtdata):
        self.append("""\
.PHONY: {tgt}
{tgt}: {out}
\t{rem} push {src} {dst}

""".format(**fmtdata))


    def copy(self, am, name):
        # don't copy if we're not on an x86 host OR
        # if 'out' is a native binary OR
        # if 'out' is a RISC-V binary (we can emulate it)
        if not X86.is_native() or am.narch.is_native() or am.narch.is_rv32():
            return ''

        out = am.bin(name)
        tgt = out + self.copy_suffix()

        srcdir = self.dstdir
        src = path(srcdir, out)
        dstdir = am.narch.get_remote_path(srcdir)
        dst = path(dstdir, out)

        fmtdata = {
            "out":      out,
            "tgt":      tgt,
            "src":      src,
            "rem":      am.narch.rem_host,
            "dst":      dst,
        }

        if GOPTS.ssh_copy():
            self._ssh_copy(fmtdata)
        else:
            self._adb_copy(fmtdata)
        return tgt


    @staticmethod
    def mk_arm_dstdir_static(dstdir):
        return ("ssh {} mkdir -p {}" if GOPTS.ssh_copy()
             else "{} shell mkdir -p {}").format(
                ARM.rem_host, ARM.get_remote_path(dstdir))


    def mk_arm_dstdir(self, name):
        tgt = name + "-arm-dstdir"

        self.append("""\
.PHONY: {0}
{0}:
\t{1}

""".format( tgt,
            self.mk_arm_dstdir_static(self.dstdir)))
        return tgt


    def run(self, name, robj, am, dep_bin=True):
        dir = self.dstdir
        bin = robj.bin(am, name)

        suffix = "-" + robj.id if robj.id else ""
        rflags = robj.rflags(am, name)
        narch = am.narch

        fmtdata = {
            "arch":     narch.name,
            "dir":      dir,
            "bin":      bin,
            "suffix":   suffix,
            "rflags":   " " + rflags if rflags else "",
            "run":      TOOLS.run,
            "dep":      " " + bin if dep_bin else "",
        }

        self.append("""\
.PHONY: {bin}{suffix}-run
{bin}{suffix}-run:{dep}
\t{run} --arch {arch} --dir {dir} {bin}{rflags}

""".format(**fmtdata))


    def xlate(self, am, _in, out):
        flags = '--sbtflags " -regs={}"'.format(am.mode)
        for flag in self.sbtflags:
            flags = flags + ' " {}"'.format(flag)
        xflags = self.xflags

        fmtdata = {
            "arch":     am.narch.name,
            "srcdir":   self.srcdir,
            "dstdir":   self.dstdir,
            "in":       _in,
            "out":      out,
            "xflags":   " " + xflags if xflags else "",
            "flags":    flags,
            "xlate":    TOOLS.xlate,
        }

        self.append("""\
.PHONY: {out}
{out}: {dstdir}/{out}

{dstdir}/{out}: {dstdir}/{in}
\t{xlate} --arch {arch} --srcdir {srcdir} --dstdir {dstdir} {in} -o {out}{xflags} {flags}

""".format(**fmtdata))


    def _diff(self, f0, f1):
        if self.out_filter:
            return (
                "\tcat {0} | {2} >{0}.filt\n" +
                "\tcat {1} | {2} >{1}.filt\n" +
                "\tdiff {0}.filt {1}.filt").format(
                    f0, f1, self.out_filter)
        else:
            return "\tdiff {0} {1}".format(f0, f1)


    def test1(self, run):
        id = run.id

        if run.outidx:
            name = lambda am: run.out(am, name=self.name)
        else:
            name = lambda am: path(self.dstdir,
                Run.build_name(am, self.name, id, Run.out_suffix()))

        # gen diffs
        diffs = []
        def diff(f0, f1):
            diffs.append(self._diff(f0, f1))

        xams = self._xams()

        for xam in xams:
            if not xam.narch.is_native():
                continue

            xout = name(xam)
            # foreign
            # skip rv32 if on arm
            if not xam.narch.is_arm():
                fam = ArchAndMode(None, xam.farch)
                fout = name(fam)
                diff(fout, xout)
            # native
            if xam.narch in self.narchs:
                nam = ArchAndMode(None, xam.narch)
                nout = name(nam)
                diff(nout, xout)

        tname = Run.build_name(None, self.name, id, None)
        fmtdata = {
            "name":     tname,
            "runs":     " ".join(self.get_all_runs(Runs([run]), self.name)),
            "diffs":    "\n".join(diffs)
        }

        self.append("""\
.PHONY: {name}-test
{name}-test: {runs}
{diffs}

""".format(**fmtdata))

        return tname + self.test_suffix()


    def test(self, runs):
        tests = []
        for run in runs:
            tests.append(self.test1(run))

        if len(tests) > 1:
            tsuf = self.test_suffix()
            self.alias(self.name + tsuf, tests)


    def measure(self, robj, dep_bin=True):
        args_str = robj.str(None)
        suffix = robj.id

        mflags = self.mflags
        if suffix:
            if not mflags:
                mflags = []
            mflags.extend(["--id", self.name + '-' + suffix])
        fmtdata = {
            "measure":  TOOLS.measure,
            "dstdir":   self.dstdir,
            "name":     self.name,
            "suffix":   "-" + suffix if suffix else "",
            "args":     " " + args_str if args_str else "",
            "stdin":    " --stdin=" + robj.stdin if robj.stdin else "",
            "mflags":   " " + " ".join(mflags) if mflags else "",
            "dep":      " " + self.name if dep_bin else "",
        }

        self.append("""\
.PHONY: {name}{suffix}-measure
{name}{suffix}-measure:{dep}
\t{measure} {dstdir} {name}{args}{stdin}{mflags}

""".format(**fmtdata))


    def alias(self, name, aliasees):
        fmtdata = {
            "name":     name,
            "aliasees": " ".join(aliasees),
        }

        self.append("""\
.PHONY: {name}
{name}: {aliasees}

""".format(**fmtdata))


    def alias_build_all(self):
        mod = self.name

        nmods = [arch.add_prefix(mod) for arch in self.narchs]
        xmods = [farch.add_prefix(narch.add_prefix(mod)) + "-" + mode
                for (farch, narch) in self.xarchs
                for mode in SBT.modes]

        fmtdata = {
            "mod":      mod,
            "nmods":    " ".join(nmods),
            "xmods":    " ".join(xmods)
        }

        self.append("""\
.PHONY: {mod}
{mod}: {nmods} {xmods}

""".format(**fmtdata))


    def _farchs(self):
        return unique([farch for (farch, narch) in self.xarchs])

    def _nams(self):
        return [ArchAndMode(None, narch) for narch in self.narchs]

    def _xams(self):
        return [ArchAndMode(farch, narch, mode)
                for (farch, narch) in self.xarchs
                for mode in SBT.modes]

    def _ufams(self):
        farchs = unique([am.farch for am in self._xams() if am.farch])
        return [ArchAndMode(farch, None) for farch in farchs]

    def _unams(self):
        narchs = unique([narch for narch in self.narchs])
        return [ArchAndMode(None, narch) for narch in narchs]

    def ams(self):
        return self._nams() + self._xams()

    def _ntgts(self, name):
        return [am.bin(name) for am in self._nams()]

    def _xtgts(self, name):
        return [am.bin(name) for am in self._xams()]

    def tgts(self, name):
        return self._ntgts(name) + self._xtgts(name)

    def apply_suffixes(self, tgts, suffixes, gsuf=None):
        a = []
        gsuf = gsuf if gsuf else ''
        for tgt in tgts:
            for suffix in suffixes:
                suf = "-" + suffix if suffix else ""
                a.append(tgt + suf + gsuf)
        return a

    @staticmethod
    def run_suffix():
        return "-run"

    @staticmethod
    def test_suffix():
        return "-test"

    @staticmethod
    def copy_suffix():
        return "-copy"

    @staticmethod
    def run_suffixes(runs):
        return [r.id if r.id else '' for r in runs]

    def get_runs(self, runs, am, name):
        return self.apply_suffixes([am.bin(name)],
                self.run_suffixes(runs),
                self.run_suffix())

    def get_all_runs(self, runs, name):
        return self.apply_suffixes(self.tgts(name),
                self.run_suffixes(runs),
                self.run_suffix())

    def get_all_tests(self, runs, name):
        return self.apply_suffixes(self.tgts(name),
                self.run_suffixes(runs),
                self.test_suffix())

    def alias_run_all(self, runs):
        fmtdata = {
            "name":     self.name,
            "runs":     " ".join(self.get_all_runs(runs, self.name)),
        }

        self.append("""\
.PHONY: {name}-run
{name}-run: {runs}

""".format(**fmtdata))


    def clean(self):
        self.append("""\
.phony: {name}-clean
{name}-clean:
\trm -rf {dstdir}

""".format(**{
        "name":     self.name,
        "dstdir":   self.dstdir}))

