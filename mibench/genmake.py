#!/usr/bin/env python3

from auto.genmake import *

# Mibench source
# http://vhosts.eecs.umich.edu/mibench//automotive.tar.gz
# http://vhosts.eecs.umich.edu/mibench//network.tar.gz
# http://vhosts.eecs.umich.edu/mibench//security.tar.gz
# http://vhosts.eecs.umich.edu/mibench//telecomm.tar.gz
# http://vhosts.eecs.umich.edu/mibench//office.tar.gz
# http://vhosts.eecs.umich.edu/mibench//consumer.tar.gz

class Bench:
    narchs = [RV32_LINUX, X86]
    xarchs = [(RV32_LINUX, X86)]
    srcdir = path(DIR.top, "mibench")
    dstdir = path(DIR.build, "mibench")
    xflags = None
    bflags = None
    rflags = "-o {}.out"

    PROLOGUE = """\
### MIBENCH ###

all: benchs

clean:
\trm -rf {}

""".format(dstdir)

    def __init__(self, name, dir, ins, args=None, dbg=False,
            stdin=None, sbtflags=[], rflags=None,
            dstdir=None):
        self.name = name
        self.dir = dir
        self.ins = ins
        self.srcdir = path(srcdir, dir)
        if dstdir == None:
            self.dstdir = path(Bench.dstdir, dir)
        else:
            self.dstdir = dstdir
        self.args = args
        self.stdin = stdin

        if stdin:
            self.rflags = cat(args, "<", stdin, Bench.rflags)
        else:
            self.rflags = cat(args, Bench.rflags)

        if dbg:
            self.xflags = cat(Bench.xflags, "--dbg")
            self.bflags = cat(Bench.bflags, "--dbg")
        self.sbtflags = sbtflags
        self.rflags = cat(self.rflags, rflags)


    def _measure(self):
        fmtdata = {
            "measure":  TOOLS.measure,
            "dstdir":   self.dstdir,
            "name":     self.name,
            "args":     " " + self.args if self.args else "",
            "stdin":    " --stdin=" + self.stdin if self.stdin else "",
        }

        return """\
.PHONY: {name}-measure
{name}-measure: {name}
\t{measure} {dstdir} {name}{args}{stdin}

""".format(**fmtdata)


    def gen(self):
        name = self.name

        # module comment
        txt = "### {} ###\n\n".format(name)

        # native builds
        for arch in self.narchs:
            out = arch.add_prefix(name)
            txt = txt + \
                bldnrun(arch, self.srcdir, self.dstdir, self.ins, out,
                    self.bflags, self.rflags.format(out))

        # translations
        for xarch in self.xarchs:
            (farch, narch) = xarch
            fmod  = farch.out2objname(name)
            nmod  = farch.add_prefix(narch.add_prefix(name))

            for mode in SBT.modes:
                txt = txt + \
                    xlatenrun(narch, self.srcdir, self.dstdir,
                        fmod, nmod, mode,
                        self.xflags, self.rflags.format(nmod + "-" + mode),
                        sbtflags=self.sbtflags)

        # tests
        txt = txt + test(self.xarchs, self.dstdir, self.name, ntest=True)

        # aliases
        txt = txt + aliases(self.narchs, self.xarchs, self.name)

        # measure
        txt = txt + self._measure()

        # clean
        txt = txt + """\
.phony: {name}-clean
{name}-clean:
\trm -rf {dstdir}

""".format(**{
        "name":     name,
        "dstdir":   self.dstdir})

        return txt


class Rijndael(Bench):
    class Args:
        def __init__(self, suffix, args):
            self.suffix = suffix
            self._args = args

        def args(self, prefix, mode):
            fmtdata = {
                "prefix":   prefix + "-" if prefix else '',
                "mode":     "-" + mode if mode else ''
            }

            args = list(self._args)
            args[0] = args[0].format(**fmtdata)
            args[1] = args[1].format(**fmtdata)
            return args


    class X:
        """ this class just holds some related data together """

        def __init__(self, farch, narch, mode=None):
            self.farch = farch
            self.narch = narch
            self.mode = mode

        def prefix(self):
            if not self.farch:
                return self.narch.prefix
            else:
                return self.farch.add_prefix(self.narch.prefix)

        def out(self, name):
            if not self.farch:
                return self.prefix() + "-" + name
            else:
                return self.prefix() + "-" + name + "-" + self.mode


    def __init__(self, name, dir, ins, args=None):
        super(Rijndael, self).__init__(name, dir, ins, args)


    def _measure(self, suffix, args):
        fmtdata = {
            "measure":  TOOLS.measure,
            "dstdir":   self.dstdir,
            "name":     self.name,
            "suffix":   suffix,
            "args":     " ".join(args),
        }

        return """\
.PHONY: {name}{suffix}-measure
{name}{suffix}-measure: {name}
\t{measure} {dstdir} {name} {args}

""".format(**fmtdata)


    def gen(self):
        name = self.name
        srcdir = self.srcdir
        dstdir = self.dstdir

        # module comment
        txt = "### {} ###\n\n".format(name)

        # native builds
        for arch in self.narchs:
            out = arch.add_prefix(name)
            txt = txt + \
                bld(arch, srcdir, dstdir, self.ins, out, self.bflags)

        # translations
        for xarch in self.xarchs:
            (farch, narch) = xarch
            fmod  = farch.out2objname(name)
            nmod  = farch.add_prefix(narch.add_prefix(name))

            for mode in SBT.modes:
                txt = txt + \
                    xlate(narch, srcdir, dstdir,
                        fmod, nmod, mode, self.xflags)

        # prepare archs and modes
        xs = [self.X(None, arch) for arch in self.narchs]
        xs.extend(
            [self.X(farch, narch, mode)
            for (farch, narch) in self.xarchs
            for mode in SBT.modes])

        # prepare args
        asc = path(srcdir, "input_large.asc")
        enc = path(dstdir, "{prefix}output_large{mode}.enc")
        dec = path(dstdir, "{prefix}output_large{mode}.dec")
        args = [
            self.Args("-encode", [
                asc,
                enc,
                "e",
                "1234567890abcdeffedcba09876543211234567890abcdeffedcba0987654321"
            ]),
            self.Args("-decode", [
                enc,
                dec,
                "d",
                "1234567890abcdeffedcba09876543211234567890abcdeffedcba0987654321"
            ])
        ]
        suffixes = [a.suffix for a in args]

        # runs and tests
        for x in xs:
            arch = x.narch
            prefix = x.prefix()
            mode = x.mode
            out = x.out(name)
            fdec = dec.format(**{
                "prefix":   prefix + "-",
                "mode":     "-" + mode if mode else ''
            })

            for a in args:
                aargs = a.args(prefix, mode)
                suffix = a.suffix

                rflags = cat(" ".join(aargs), self.rflags.format(out + suffix))
                txt = txt + run(arch, dstdir, out, rflags, suffix)

            # runs + test
            runs = [out + suffix + "-run" for suffix in suffixes]
            txt = txt + """\
.PHONY: {out}-run
{out}-run: {runs}

.PHONY: {out}-test
{out}-test: {runs}
\tdiff {dec} {asc}

""".format(**{
        "out":  out,
        "runs": " ".join(runs),
        "dec":  fdec,
        "asc":  asc,
    })


        # aliases
        txt = txt + aliases(self.narchs, self.xarchs, self.name)

        # measures
        for a in args:
            aargs = a.args('', '')
            suffix = a.suffix
            txt = txt + self._measure(suffix, aargs)

        # all tests & measures targets
        txt = txt + """\
.PHONY: {name}-test
{name}-test: {tests}

.PHONY: {name}-measure
{name}-measure: {measures}

""".format(**{
        "name":     name,
        "tests":    " ".join([x.out(name) + "-test" for x in xs]),
        "measures": " ".join([name + suffix + "-measure" for suffix in suffixes])
    })

        # clean
        txt = txt + """\
.phony: {name}-clean
{name}-clean:
\trm -rf {dstdir}

""".format(**{
        "name":     name,
        "dstdir":   dstdir})

        return txt



if __name__ == "__main__":
    srcdir = Bench.srcdir
    dstdir = Bench.dstdir

    benchs = [
        Bench("dijkstra", "network/dijkstra",
            ["dijkstra_large.c"],
            path(srcdir, "network/dijkstra/input.dat")),
        Bench("crc32", "telecomm/CRC32",
            ["crc_32.c"],
            path(srcdir, "telecomm/adpcm/data/large.pcm")),
        Rijndael("rijndael", "security/rijndael",
            ["aes.c", "aesxam.c"]),
        Bench("sha", "security/sha",
            ["sha_driver.c", "sha.c"],
            path(srcdir, "security/sha/input_large.asc"),
            sbtflags=["-stack-size=16384"]),
        Bench("rawcaudio", "telecomm/adpcm/src",
            ["rawcaudio.c", "adpcm.c"],
            dstdir=path(Bench.dstdir, "telecomm/adpcm/rawcaudio"),
            stdin=path(srcdir, "telecomm/adpcm/data/large.pcm"),
            rflags="--bin"),
        Bench("rawdaudio", "telecomm/adpcm/src",
            ["rawdaudio.c", "adpcm.c"],
            dstdir=path(Bench.dstdir, "telecomm/adpcm/rawdaudio"),
            stdin=path(srcdir, "telecomm/adpcm/data/large.adpcm"),
            rflags="--bin"),
        Bench("stringsearch", "office/stringsearch",
            ["bmhasrch.c", "bmhisrch.c", "bmhsrch.c", "pbmsrch_large.c"],
            sbtflags=["-stack-size=131072"]),
    ]

    txt = Bench.PROLOGUE
    for bench in benchs:
        txt = txt + bench.gen()

    txt = txt + """\
.PHONY: benchs
benchs: {}

.PHONY: benchs-test
benchs-test: benchs {}

.PHONY: benchs-measure
benchs-measure: benchs {}
""".format(
        " ".join([b.name for b in benchs]),
        " ".join([b.name + "-test" for b in benchs]),
        " ".join([b.name + "-measure" for b in benchs]),
    )

    # write txt to Makefile
    with open("Makefile", "w") as f:
        f.write(txt)


"""
## 01- BASICMATH
# rv32: OK (soft-float)

BASICMATH_NAME  := basicmath
BASICMATH_DIR   := automotive/basicmath
BASICMATH_MODS  := basicmath_large rad2deg cubic isqrt
BASICMATH_ARGS  :=

## 02- BITCOUNT
# rv32: OK (soft-float)

BITCOUNT_NAME   := bitcount
BITCOUNT_DIR    := automotive/bitcount
BITCOUNT_MODS   := bitcnt_1 bitcnt_2 bitcnt_3 bitcnt_4 \
                   bitcnts bitfiles bitstrng bstr_i
BITCOUNT_ARGS   := 1125000 | sed 's/Time:[^;]*; //;/^Best/d;/^Worst/d'

## 03- SUSAN
# rv32: OK (soft-float)

SUSAN_NAME      := susan
SUSAN_DIR       := automotive/susan
SUSAN_MODS      := susan
SUSAN_ARGS      := notests

## 04- PATRICIA
# rv32: OK (soft float)

PATRICIA_NAME   := patricia
PATRICIA_DIR    := network/patricia
PATRICIA_MODS   := patricia patricia_test
PATRICIA_ARGS   := $(MIBENCH)/$(PATRICIA_DIR)/large.udp

## 07- BLOWFISH
# rv32: wrong results

BLOWFISH_NAME   := blowfish
BLOWFISH_DIR    := security/blowfish
BLOWFISH_MODS   := bf bf_skey bf_ecb bf_enc bf_cbc bf_cfb64 bf_ofb64
BLOWFISH_ARGS   := notests

## 12- FFT
# rv32: OK (soft-float)

FFT_NAME        := fft
FFT_DIR         := telecomm/FFT
FFT_MODS        := main fftmisc fourierf
FFT_ARGS        := notests

## 14- LAME
# rv32: OK (soft-float)

LAME_NAME := lame
LAME_DIR  := consumer/lame
LAME_SRC_DIR_SUFFIX := /lame3.70

LAME_MODS := \
        main \
        brhist \
        formatBitstream \
        fft \
        get_audio \
        l3bitstream \
        id3tag \
        ieeefloat \
        lame \
        newmdct \
        parse \
        portableio \
        psymodel \
        quantize \
        quantize-pvt \
        vbrquantize \
        reservoir \
        tables \
        takehiro \
        timestatus \
        util \
        VbrTag \
        version \
        gtkanal \
        gpkplotting \
        mpglib/common \
        mpglib/dct64_i386 \
        mpglib/decode_i386 \
        mpglib/layer3 \
        mpglib/tabinit \
        mpglib/interface \
        mpglib/main

LAME_CFLAGS := -DLAMEPARSE -DLAMESNDFILE
LAME_DEPS := $(BUILD_MIBENCH)/$(LAME_DIR)/mpglib

LAME_ARGS := notests
"""

