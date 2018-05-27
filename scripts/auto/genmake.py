#!/usr/bin/env python3

from auto.config import SBT, TOOLS
from auto.utils import cat, path

class Module:
    def __init__(self, name, src, xflags=None, bflags=None, rflags=None):
        self.name = name
        self.src = src
        self.xflags = xflags
        self.bflags = bflags
        self.rflags = rflags


class ArchAndMode:
    def __init__(self, farch, narch, mode=None):
        self.farch = farch
        self.narch = narch
        self.mode = mode

    # get arch prefix string
    def prefix(self):
        if not self.farch:
            return self.narch.prefix
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

        return templ.format(**{
            "prefix":   prefix,
            "mode":     mode,
            })


# arguments for a given run
class Run:
    def __init__(self, args, id=None, rflags=None, outidx=None, stdin=None):
        self.args = args
        self.rflags_arg = rflags
        self.id = id
        self.outidx = outidx
        self.stdin = None


    def bin(self, am, name):
        return am.bin(name)


    def out(self):
        if self.outidx == None:
            return None
        return self.args[self.outidx]


    def build(self, am):
        prefix = am.prefix() if am else ''
        mode = am.mode if am else None

        fmtdata = {
            "prefix":   prefix,
            "mode":     "-" + mode if mode else ''
        }

        return [arg.format(**fmtdata) for arg in self.args]


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


    def rflags(self, am):
        args_str = self.str(am)

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
            xflags, bflags, rflags, mflags, sbtflags=[]):
        self.narchs = narchs
        self.xarchs = xarchs
        self.srcdir = srcdir
        self.dstdir = dstdir
        self.name = name
        self.xflags = xflags
        self.bflags = bflags
        self.rflags = rflags
        self.mflags = mflags
        self.sbtflags = sbtflags
        #
        self.stdin = None
        self.out_filter = None
        #
        self.txt = "### {} ###\n\n".format(name)


    def append(self, txt):
        self.txt = self.txt + txt


    def _srcname(self, templ, arch):
        return templ.format(arch.prefix)


    def do_mod(self, name, src):
        # native builds
        for arch in self.narchs:
            _in = self._srcname(src, arch)
            out = arch.add_prefix(name)
            # rflags.format(out)
            self.bldnrun(arch, [_in], out)

        # translations
        for (farch, narch) in self.xarchs:
            fmod = farch.out2objname(name)
            nmod = farch.add_prefix(narch.add_prefix(name))

            # rflags.format(nmod + "-" + mode)
            for mode in SBT.modes:
                self.xlatenrun(narch, fmod, nmod, mode)

        # tests
        self.test(name, ntest=len(self.narchs) > 1)

        # aliases
        self.alias_build_all(name)
        self.alias_run_all(name)

        return self.txt


    def bldnrun(self, arch, ins, out):
        self.bld(arch, ins, out)
        self.run(arch, out)


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


    def run(self, name, robj, am):
        dir = self.dstdir

        suffix = "-" + robj.id if robj.id else ""
        rflags = robj.rflags(am)
        narch = am.narch

        fmtdata = {
            "arch":     narch.name,
            "dir":      dir,
            "bin":      robj.bin(am, name),
            "suffix":   suffix,
            "rflags":   " " + rflags if rflags else "",
            "run":      TOOLS.run,
        }

        self.append("""\
.PHONY: {bin}{suffix}-run
{bin}{suffix}-run: {bin}
\t{run} --arch {arch} --dir {dir} {bin}{rflags}

""".format(**fmtdata))


    def xlate(self, arch, _in, out, mode):
        flags = '--sbtflags " -regs={}"'.format(mode)
        for flag in self.sbtflags:
            flags = flags + ' " {}"'.format(flag)
        xflags = self.xflags

        fmtdata = {
            "arch":     arch.name,
            "srcdir":   self.srcdir,
            "dstdir":   self.dstdir,
            "in":       _in,
            "out":      out + "-" + mode,
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


    def xlatenrun(self, arch, _in, out, mode):
        self.xlate(arch, _in, out, mode)
        self.run(arch, out + "-" + mode)


    def test(self, ntest=False, id=None, outname=None):
        name = self.name
        dir = self.dstdir

        def build_name(base, prefix, mode, id, suffix):
            r = base
            if prefix:
                r = prefix + '-' + r
            if mode:
                r = r + '-' + mode
            if id:
                r = r + '-' + id
            if suffix:
                r = r + suffix
            return r

        def fmt_name(base, prefix, mode):
            return base.format(**{
                "prefix" : prefix + '-',
                "mode"   : '-' + mode if mode else ''})

        def xprefix(farch, narch):
            return farch.prefix + '-' + narch.prefix

        if outname:
            fname = lambda prefix: fmt_name(outname, prefix, "")
            nname = fname
            xname = lambda prefix, mode: fmt_name(outname, prefix, mode)
        else:
            outname = name
            suffix = ".out"
            fname = lambda prefix: path(dir,
                        build_name(outname, prefix, None, id, suffix))
            nname = fname
            xname = lambda prefix, mode: path(dir,
                        build_name(outname, prefix, mode, id, suffix))

        diffs = []
        def diff(out1, out2):
            if self.out_filter:
                diff = (
                    "\tcat {0} | {2} >{0}.filt\n" +
                    "\tcat {1} | {2} >{1}.filt\n" +
                    "\tdiff {0}.filt {1}.filt").format(
                        out1, out2, self.out_filter)
            else:
                diff = "\tdiff {0} {1}".format(out1, out2)
            diffs.append(diff)


        farchs = []
        narchs = []
        for xarch in self.xarchs:
            (farch, narch) = xarch
            farchs.append(farch)
            narchs.append(narch)

            fout = fname(farch.prefix)

            for mode in SBT.modes:
                xout = xname(xprefix(farch, narch), mode)
                diff(fout, xout)
                if ntest:
                    nout = nname(narch.prefix)
                    diff(nout, xout)

        def run_name(prefix):
            return build_name(name, prefix, None, id, "-run")

        def xrun_name(prefix, mode):
            return build_name(name, prefix, mode, id, "-run")

        runs = [run_name(arch.prefix)
                for arch in farchs + (narchs if ntest else [])]

        runs.extend(
            [xrun_name(xprefix(farch, narch), mode)
                for (farch, narch) in self.xarchs
                for mode in SBT.modes])

        fmtdata = {
            "name":     build_name(name, None, None, id, None),
            "runs":     " ".join(runs),
            "diffs":    "\n".join(diffs)
        }

        self.append("""\
.PHONY: {name}-test
{name}-test: {runs}
{diffs}

""".format(**fmtdata))


    def measure(self, robj):
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
            "stdin":    " --stdin=" + self.stdin if self.stdin else "",
            "mflags":   " " + " ".join(mflags) if mflags else "",
        }

        return """\
.PHONY: {name}{suffix}-measure
{name}{suffix}-measure: {name}
\t{measure} {dstdir} {name}{args}{stdin}{mflags}

""".format(**fmtdata)


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


    def _nams(self):
        return [ArchAndMode(None, narch) for narch in self.narchs]

    def _xams(self):
        return [ArchAndMode(farch, narch, mode)
                for (farch, narch) in self.xarchs
                for mode in SBT.modes]

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
        for tgt in tgts:
            for suffix in suffixes:
                suf = "-" + suffix if suffix else ""
                a.append(cat(tgt, suf, gsuf))
        return a

    @staticmethod
    def run_suffix():
        return "-run"

    @staticmethod
    def test_suffix():
        return "-test"

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

