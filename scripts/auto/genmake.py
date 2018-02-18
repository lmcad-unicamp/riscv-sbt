#!/usr/bin/env python3

from auto.config import *
from auto.utils import *


def bld(arch, srcdir, dstdir, ins, out, bflags=None):
    if len(ins) == 1:
        objs = [arch.out2objname(out)]
        aobjs = [path(dstdir, objs[0])]
    else:
        objs = [arch.src2objname(src) for src in ins]
        aobjs = [path(dstdir, obj) for obj in objs]

    ains = [path(srcdir, i) for i in ins]

    fmtdata = {
        "arch":     arch.name,
        "aobjs":    " ".join(aobjs),
        "srcdir":   srcdir,
        "dstdir":   dstdir,
        "ins":      " ".join(ins),
        "ains":     " ".join(ains),
        "out":      out,
        "bflags":   " " + bflags if bflags else "",
        "build":    TOOLS.build,
    }

    txt = """\
.PHONY: {out}
{out}: {dstdir}/{out}

{dstdir}/{out} {aobjs}: {ains}
	{build} --arch {arch} --srcdir {srcdir} --dstdir {dstdir} {ins} -o {out}{bflags}

""".format(**fmtdata)

    return txt


def run(arch, dir, out, rflags=None, suffix=""):
    if rflags and len(suffix) > 0:
        rflags = rflags.replace(".out", suffix + ".out")
    fmtdata = {
        "arch":     arch.name,
        "dir":      dir,
        "out":      out,
        "suffix":   suffix,
        "rflags":   " " + rflags if rflags else "",
        "run":      TOOLS.run,
    }

    return """\
.PHONY: {out}{suffix}-run
{out}{suffix}-run: {out}
	{run} --arch {arch} --dir {dir} {out}{rflags}

""".format(**fmtdata)


def bldnrun(arch, srcdir, dstdir, ins, out, bflags=None, rflags=None):
    txt = bld(arch, srcdir, dstdir, ins, out, bflags)
    txt = txt + run(arch, dstdir, out, rflags)
    return txt


def xlate(arch, srcdir, dstdir, _in, out, mode, xflags=None, sbtflags=[]):
    flags = '--flags " -regs={}"'.format(mode)
    for flag in sbtflags:
        flags = flags + ' " {}"'.format(flag)

    fmtdata = {
        "arch":     arch.name,
        "srcdir":   srcdir,
        "dstdir":   dstdir,
        "in":       _in,
        "out":      out + "-" + mode,
        "xflags":   " " + xflags if xflags else "",
        "flags":    flags,
        "xlate":    TOOLS.xlate,
    }

    return """\
.PHONY: {out}
{out}: {dstdir}/{out}

{dstdir}/{out}: {dstdir}/{in}
	{xlate} --arch {arch} --srcdir {srcdir} --dstdir {dstdir} {in} -o {out}{xflags} {flags}

""".format(**fmtdata)


def xlatenrun(arch, srcdir, dstdir, _in, out, mode, xflags=None, rflags=None,
        sbtflags=[]):
    txt = xlate(arch, srcdir, dstdir, _in, out, mode, xflags, sbtflags)
    txt = txt + run(arch, dstdir, out + "-" + mode, rflags)
    return txt


def test(xarchs, dir, name, ntest=False, out_filter=None, id=None):
    diffs = []
    def diff(bin1, bin2):
        if out_filter:
            diff = (
                "\tcat {0}/{1}.out | {3} >{0}/{1}.filt.out\n" +
                "\tcat {0}/{2}.out | {3} >{0}/{2}.filt.out\n" +
                "\tdiff {0}/{1}.filt.out {0}/{2}.filt.out").format(
                    dir, bin1, bin2, out_filter)
        else:
            diff = "\tdiff {0}/{1}.out {0}/{2}.out".format(dir, bin1, bin2)
        diffs.append(diff)

    if id:
        fname = name + '-' + id
        nname = name + '-' + id
        xname = lambda mode: name + '-' + mode + '-' + id
    else:
        fname = name
        nname = name
        xname = lambda mode: name + '-' + mode

    farchs = []
    narchs = []
    for xarch in xarchs:
        (farch, narch) = xarch
        farchs.append(farch)
        narchs.append(narch)

        fbin = farch.add_prefix(fname)

        for mode in SBT.modes:
            xbin = farch.add_prefix(narch.add_prefix(xname(mode)))
            diff(fbin, xbin)
            if ntest:
                nbin = narch.add_prefix(nname)
                diff(nbin, xbin)

    runs = [arch.add_prefix(fname + "-run")
            for arch in farchs + (narchs if ntest else [])]

    runs.extend(
        [farch.add_prefix(
            narch.add_prefix(xname(mode) + "-run"))
            for (farch, narch) in xarchs
            for mode in SBT.modes])

    fmtdata = {
        "name":     fname,
        "runs":     " ".join(runs),
        "diffs":    "\n".join(diffs)
    }

    txt = """\
.PHONY: {name}-test
{name}-test: {runs}
{diffs}

""".format(**fmtdata)

    return txt


def alias(name, aliasees):
    fmtdata = {
        "name":     name,
        "aliasees": " ".join(aliasees),
    }

    return """\
.PHONY: {name}
{name}: {aliasees}

""".format(**fmtdata)


def alias_build_all(narchs, xarchs, mod):
    nmods = [arch.add_prefix(mod) for arch in narchs]
    xmods = [farch.add_prefix(narch.add_prefix(mod)) + "-" + mode
            for (farch, narch) in xarchs
            for mode in SBT.modes]

    fmtdata = {
        "mod":      mod,
        "nmods":    " ".join(nmods),
        "xmods":    " ".join(xmods)
    }

    return """\
.PHONY: {mod}
{mod}: {nmods} {xmods}

""".format(**fmtdata)


def alias_run_all(narchs, xarchs, mod, suffixes=['']):
    nmods = [arch.add_prefix(mod) for arch in narchs]
    xmods = [farch.add_prefix(narch.add_prefix(mod)) + "-" + mode
            for (farch, narch) in xarchs
            for mode in SBT.modes]

    runs = []
    for m in nmods:
        for suffix in suffixes:
            suffix = "-" + suffix if suffix else ""
            runs.append(m + suffix + "-run")
    for m in xmods:
        for suffix in suffixes:
            suffix = "-" + suffix if suffix else ""
            runs.append(m + suffix + "-run")

    fmtdata = {
        "mod":      mod,
        "runs":     " ".join(runs),
    }

    return """\
.PHONY: {mod}-run
{mod}-run: {runs}

""".format(**fmtdata)


class Module:
    def __init__(self, name, src, xflags=None, bflags=None, rflags=None):
        self.name = name
        self.src = src
        self.xflags = xflags
        self.bflags = bflags
        self.rflags = rflags



def do_mod(narchs, xarchs, name, src, srcdir, dstdir, xflags, bflags, rflags):
    srcname = lambda templ, arch: templ.format(arch.prefix)

    # module comment
    txt = "### {} ###\n\n".format(name)

    # native builds
    for arch in narchs:
        _in = srcname(src, arch)
        out = arch.add_prefix(name)
        txt = txt + \
            bldnrun(arch, srcdir, dstdir, [_in], out, bflags, rflags.format(out))

    # translations
    for (farch, narch) in xarchs:
        fmod  = farch.out2objname(name)
        nmod  = farch.add_prefix(narch.add_prefix(name))

        for mode in SBT.modes:
            txt = txt + \
                xlatenrun(narch, srcdir, dstdir, fmod, nmod,
                    mode, xflags, rflags.format(nmod + "-" + mode))

    # tests
    txt = txt + test(xarchs, dstdir, name, ntest=len(narchs) > 1)

    # aliases
    txt = txt + alias_build_all(narchs, xarchs, name)
    txt = txt + alias_run_all(narchs, xarchs, name)

    return txt

