#!/usr/bin/env python3

from auto.genmake import *

# Mibench source
# http://vhosts.eecs.umich.edu/mibench//automotive.tar.gz
# http://vhosts.eecs.umich.edu/mibench//network.tar.gz
# http://vhosts.eecs.umich.edu/mibench//security.tar.gz
# http://vhosts.eecs.umich.edu/mibench//telecomm.tar.gz
# http://vhosts.eecs.umich.edu/mibench//office.tar.gz
# http://vhosts.eecs.umich.edu/mibench//consumer.tar.gz

# arguments for a given run
class Args:
    def __init__(self, args, id=None):
        self.id = id
        self._args = args


    # add prefix and mode to arguments, if needed
    def args(self, prefix, mode):
        fmtdata = {
            "prefix":   prefix + "-" if prefix else '',
            "mode":     "-" + mode if mode else ''
        }

        return [arg.format(**fmtdata) for arg in self._args]


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
    def out(self, name):
        return (self.prefix() + "-" + name +
            ("-" + self.mode if self.mode else ""));


class Bench:
    narchs = [RV32_LINUX, X86]
    xarchs = [(RV32_LINUX, X86)]
    srcdir = path(DIR.top, "mibench")
    dstdir = path(DIR.build, "mibench")
    xflags = None
    bflags = None
    rflags_var = "-o {}.out"

    PROLOGUE = """\
### MIBENCH ###

all: benchs

clean:
\trm -rf {}

""".format(dstdir)

    # args: list of Args
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
        self.args = args if args else [Args([])]
        self.stdin = stdin
        self.sbtflags = sbtflags
        self.dbg = dbg
        self.rflags_suffix = rflags

        if dbg:
            self.bflags = cat(Bench.bflags, "--dbg")
            self.xflags = cat(Bench.xflags, "--dbg")


    def rflags(self, args_obj, prefix, mode):
        if args_obj:
            args_str = " ".join(args_obj.args(prefix, mode))
        else:
            args_str = ''

        if self.stdin:
            ret = cat(args_str, "<", self.stdin, Bench.rflags_var)
        else:
            ret = cat(args_str, Bench.rflags_var)
        ret = cat(ret, self.rflags_suffix).format(
                prefix + "-" + self.name + ("-" + mode if mode else ""))
        return ret


    def gen_build(self):
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
                        fmod, nmod, mode, self.xflags,
                        sbtflags=self.sbtflags)
        return txt


    def gen_run(self):
        txt = ''

        # prepare archs and modes
        ams = [ArchAndMode(None, arch) for arch in self.narchs]
        ams.extend(
            [ArchAndMode(farch, narch, mode)
            for (farch, narch) in self.xarchs
            for mode in SBT.modes])

        # runs
        for am in ams:
            arch = am.narch
            prefix = am.prefix()
            mode = am.mode
            out = am.out(self.name)

            # runs
            for args_obj in self.args:
                suffix = "-" + args_obj.id if args_obj.id else ""
                # run
                txt = txt + run(arch, self.dstdir, out,
                        self.rflags(args_obj, prefix, mode), suffix)
        return txt


    def gen_test(self):
        return test(self.xarchs, self.dstdir, self.name, ntest=True)


    def measure(self, args_obj=None):
        suffix = None
        if args_obj:
            args = " " + " ".join(args_obj.args(prefix='', mode=''))
            suffix = args_obj.id
        else:
            args = ""

        fmtdata = {
            "measure":  TOOLS.measure,
            "dstdir":   self.dstdir,
            "name":     self.name,
            "suffix":   "-" + suffix if suffix else "",
            "args":     args,
            "stdin":    " --stdin=" + self.stdin if self.stdin else "",
        }

        return """\
.PHONY: {name}{suffix}-measure
{name}{suffix}-measure: {name}
\t{measure} {dstdir} {name}{args}{stdin}

""".format(**fmtdata)


    def gen_measure(self):
        txt = ""

        # measures
        measures = []
        for args_obj in self.args:
            txt = txt + self.measure(args_obj)
            m = self.name
            if args_obj.id:
                m = m + "-" + args_obj.id
            m = m + "-measure"
            measures.append(m)

        if len(measures) > 1:
            txt = txt + alias(self.name + "-measure", measures)

        return txt


    def gen_clean(self):
        # clean
        return """\
.phony: {name}-clean
{name}-clean:
\trm -rf {dstdir}

""".format(**{
        "name":     self.name,
        "dstdir":   self.dstdir})


    def gen_alias(self):
        # aliases
        txt = alias_build_all(self.narchs, self.xarchs, self.name)
        suffixes = [o.id if o.id else '' for o in self.args]
        txt = txt + alias_run_all(self.narchs, self.xarchs, self.name,
                suffixes)
        return txt


    def gen(self):
        txt = self.gen_build()
        txt = txt + self.gen_run()
        txt = txt + self.gen_test()
        txt = txt + self.gen_measure()
        txt = txt + self.gen_clean()
        txt = txt + self.gen_alias()
        return txt


class EncDecBench(Bench):
    def set_asc_index(self, index):
        self.asc_index = index
        return self

    def set_dec_index(self, index):
        self.dec_index = index
        return self

    def test1(self, prefix, mode, suffixes, asc, dec):
        out = prefix + "-" + self.name + ("-" + mode if mode else '')
        runs = [out + "-" + suf + "-run" for suf in suffixes]

        return (
            ".PHONY: {out}-test\n" +
            "{out}-test: {runs}\n" +
            "\tdiff {dec} {asc}\n\n").format(
            **{
                "out":  out,
                "runs": " ".join(runs),
                "asc":  asc,
                "dec":  dec,
            })


    def gen_test(self):
        txt = ''
        asc_i = self.asc_index[0]
        asc_j = self.asc_index[1]
        dec_i = self.dec_index[0]
        dec_j = self.dec_index[1]

        suffixes = [o.id for o in self.args]
        tests = []
        for arch in self.narchs + self.xarchs:
            is_xarch = isinstance(arch, tuple)
            if is_xarch:
                (farch, narch) = arch
                prefix = farch.prefix + "-" + narch.prefix
                modes = SBT.modes
            else:
                prefix = arch.prefix
                modes = ['']

            for mode in modes:
                asc = self.args[asc_i].args(prefix, mode)[asc_j]
                dec = self.args[dec_i].args(prefix, mode)[dec_j]
                txt = txt + self.test1(prefix, mode, suffixes, asc, dec)
                tests.append(prefix + "-" + self.name +
                    ("-" + mode if mode else "") + "-test")

        txt = txt + alias(self.name + "-test", tests)
        return txt


if __name__ == "__main__":
    srcdir = Bench.srcdir
    dstdir = Bench.dstdir

    # rijndael args
    rijndael_dir = "security/rijndael"
    rijndael_asc = mpath(srcdir, rijndael_dir, "input_large.asc")
    rijndael_enc = mpath(dstdir, rijndael_dir, "{prefix}output_large{mode}.enc")
    rijndael_dec = mpath(dstdir, rijndael_dir, "{prefix}output_large{mode}.dec")
    rijndael_key = "1234567890abcdeffedcba09876543211234567890abcdeffedcba0987654321"
    rijndael_args = [
        Args([rijndael_asc, rijndael_enc, "e", rijndael_key], "encode"),
        Args([rijndael_enc, rijndael_dec, "d", rijndael_key], "decode")]

    # blowfish args
    bf_dir = "security/blowfish"
    bf_asc = mpath(srcdir, bf_dir, "input_large.asc")
    bf_enc = mpath(dstdir, bf_dir, "{prefix}output_large{mode}.enc")
    bf_dec = mpath(dstdir, bf_dir, "{prefix}output_large{mode}.asc")
    bf_key = "1234567890abcdeffedcba0987654321"
    bf_args = [
        Args(["e", bf_asc, bf_enc, bf_key], "encode"),
        Args(["d", bf_enc, bf_dec, bf_key], "decode")]

    benchs = [
        Bench("dijkstra", "network/dijkstra",
            ["dijkstra_large.c"],
            [Args([path(srcdir, "network/dijkstra/input.dat")])]),
        Bench("crc32", "telecomm/CRC32",
            ["crc_32.c"],
            [Args([path(srcdir, "telecomm/adpcm/data/large.pcm")])]),
        EncDecBench("rijndael", rijndael_dir,
            ["aes.c", "aesxam.c"],
            rijndael_args).
            set_asc_index([0, 0]).
            set_dec_index([1, 1]),
        Bench("sha", "security/sha",
            ["sha_driver.c", "sha.c"],
            [Args([path(srcdir, "security/sha/input_large.asc")])],
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
        EncDecBench("blowfish", bf_dir,
            ["bf.c",
            "bf_skey.c",
            "bf_ecb.c",
            "bf_enc.c",
            "bf_cbc.c",
            "bf_cfb64.c",
            "bf_ofb64.c"],
            bf_args,
            sbtflags=["-stack-size=131072"],
            rflags="--exp-rc=1",
            dbg=True).
            set_asc_index([0, 1]).
            set_dec_index([1, 2]),
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

