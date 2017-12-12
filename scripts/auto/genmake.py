#!/usr/bin/env python3

from auto.config import *
from auto.utils import *


def bldnrun(arch, srcdir, dstdir, ins, out, bflags=None, rflags=None):
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
        "rflags":   " " + rflags if rflags else "",
        "build":    TOOLS.build,
        "run":      TOOLS.run,
    }

    txt = """\
.PHONY: {out}
{out}: {dstdir}/{out}

{dstdir}/{out} {aobjs}: {ains}
	{build} --arch {arch} --srcdir {srcdir} --dstdir {dstdir} {ins} -o {out}{bflags}

.PHONY: {out}-run
{out}-run: {out}
	{run} --arch {arch} --dir {dstdir} {out}{rflags}

""".format(**fmtdata)

    return txt


def xlatenrun(arch, srcdir, dstdir, _in, out, mode, xflags=None, rflags=None):
    flags = '--flags " -regs={}"'.format(mode)

    fmtdata = {
        "arch":     arch.name,
        "srcdir":   srcdir,
        "dstdir":   dstdir,
        "in":       _in,
        "out":      out + "-" + mode,
        "mode":     mode,
        "xflags":   " " + xflags if xflags else "",
        "rflags":   " " + rflags if rflags else "",
        "flags":    flags,
        "xlate":    TOOLS.xlate,
        "run":      TOOLS.run,
    }

    txt = """\
.PHONY: {out}
{out}: {dstdir}/{out}

{dstdir}/{out}: {dstdir}/{in}
	{xlate} --arch {arch} --srcdir {srcdir} --dstdir {dstdir} {in} -o {out}{xflags} {flags}

.PHONY: {out}-run
{out}-run: {out}
	{run} --arch {arch} --dir {dstdir} {out}{rflags}

""".format(**fmtdata)

    return txt


def test(xarchs, dir, name, ntest=False):
    diffs = []
    def diff(bin1, bin2):
        diff = "\tdiff {0}/{1}.out {0}/{2}.out".format(dir, bin1, bin2)
        diffs.append(diff)

    farchs = []
    narchs = []
    for xarch in xarchs:
        (farch, narch) = xarch
        farchs.append(farch)
        narchs.append(narch)

        fbin = farch.add_prefix(name)

        for mode in SBT.modes:
            xbin = farch.add_prefix(narch.add_prefix(name + "-" + mode))
            diff(fbin, xbin)
            if ntest:
                nbin = narch.add_prefix(name)
                diff(nbin, xbin)

    runs = [arch.add_prefix(name + "-run")
            for arch in farchs + (narchs if ntest else [])]

    runs.extend(
        [farch.add_prefix(
            narch.add_prefix(name + "-" + mode + "-run"))
            for (farch, narch) in xarchs
            for mode in SBT.modes])

    fmtdata = {
        "name":     name,
        "runs":     " ".join(runs),
        "diffs":    "\n".join(diffs)
    }

    txt = """\
.PHONY: {name}-test
{name}-test: {runs}
{diffs}

""".format(**fmtdata)

    return txt


def aliases(narchs, xarchs, mod):
    nmods = [arch.add_prefix(mod) for arch in narchs]
    xmods = [farch.add_prefix(narch.add_prefix(mod)) + "-" + mode
            for (farch, narch) in xarchs
            for mode in SBT.modes]

    runs = [m + "-run" for m in nmods + xmods]

    fmtdata = {
        "mod":      mod,
        "nmods":    " ".join(nmods),
        "xmods":    " ".join(xmods),
        "runs":     " ".join(runs),
    }

    return """\
.PHONY: {mod}
{mod}: {nmods} {xmods}

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
    txt = txt + aliases(narchs, xarchs, name)

    return txt

