#!/usr/bin/env python3

from auto.config import DIR, RV32_LINUX, SBT, TOOLS, X86
from auto.genmake import ArchAndMode, GenMake, Run, Runs
from auto.utils import cat, mpath, path, xassert

# Mibench source
# http://vhosts.eecs.umich.edu/mibench//automotive.tar.gz
# http://vhosts.eecs.umich.edu/mibench//network.tar.gz
# http://vhosts.eecs.umich.edu/mibench//security.tar.gz
# http://vhosts.eecs.umich.edu/mibench//telecomm.tar.gz
# http://vhosts.eecs.umich.edu/mibench//office.tar.gz
# http://vhosts.eecs.umich.edu/mibench//consumer.tar.gz


class Bench:
    def __init__(self, name, dir, ins, runs=None, dbg=None,
            bflags=None, xflags=None, sbtflags=[], mflags=None,
            srcdir=None, dstdir=None, narchs=None, xarchs=None):
        self.name = name
        self.dir = dir
        self.ins = ins
        self.runs = runs
        self.runs.name = name
        self.sbtflags = sbtflags
        self.dbg = dbg

        xflags = cat(bflags, xflags)
        if dbg == "dbg":
            bflags = cat(bflags, "--dbg")
            xflags = cat(xflags, "--dbg")
        if dbg == "opt":
            bflags = cat(bflags, "--dbg --opt")
            xflags = cat(xflags, "--dbg --opt")
        self.narchs = narchs
        self.xarchs = xarchs

        self.gm = GenMake(narchs, xarchs,
                srcdir, dstdir, self.name,
                xflags, bflags, mflags, self.sbtflags)


    def out_filter(self, of):
        self.gm.out_filter = of
        return self


    def _gen_xinout(self, am):
        name = self.name
        use_native_sysroot = False
        out = am.bin(name)

        if use_native_sysroot:
            arch = ARCH[am.farch.prefix + "-for-" + am.narch.prefix]
            obj = arch.out2objname(name)
            # build rv32 binary for translation
            self.gm.bld(arch, self.ins, obj)
        else:
            obj = am.farch.out2objname(name)

        return (obj, out)


    def gen_build(self):
        # native builds
        for arch in self.narchs:
            out = arch.add_prefix(self.name)
            self.gm.bld(arch, self.ins, out)

        # translations
        for xarch in self.xarchs:
            for mode in SBT.modes:
                (farch, narch) = xarch
                am = ArchAndMode(farch, narch, mode)
                (fin, fout) = self._gen_xinout(am)
                self.gm.xlate(am, fin, fout)


    def gen_run(self):
        # prepare archs and modes
        ams = [ArchAndMode(None, arch) for arch in self.narchs]
        ams.extend(
            [ArchAndMode(farch, narch, mode)
            for (farch, narch) in self.xarchs
            for mode in SBT.modes])

        # runs
        for am in ams:
            for run in self.runs:
                self.gm.run(self.runs.name, run, am)


    def gen_test(self):
        return self.gm.test(self.runs)


    def gen_measure(self):
        # measures
        measures = []
        for run in self.runs:
            self.gm.measure(run)
            m = self.name
            if run.id:
                m = m + "-" + run.id
            m = m + "-measure"
            measures.append(m)

        if len(measures) > 1:
            self.gm.alias(self.name + "-measure", measures)


    def gen_alias(self):
        # aliases
        self.gm.alias_build_all()
        self.gm.alias_run_all(self.runs)


    def gen(self):
        self.gen_build()
        self.gen_run()
        self.gen_test()
        self.gen_measure()
        self.gm.clean()
        self.gen_alias()
        return self.gm.txt


class EncDecBench(Bench):
    def gen_single_test(self, tgt, runs, asc, dec):
        fmtdata = {
            "tgt":  tgt,
            "runs": " ".join(runs),
            "asc":  asc,
            "dec":  dec,
        }

        self.gm.append("""\
.PHONY: {tgt}-test
{tgt}-test: {runs}
\tdiff {dec} {asc}

""".format(**fmtdata))

        return tgt + self.gm.test_suffix()


    def gen_test(self):
        asc = self.runs.asc
        dec = self.runs.dec
        if not asc or not dec:
            raise Exception("Missing asc/dec")

        tests = []
        for am in self.gm.ams():
            tgt = am.bin(self.name)
            runs = self.gm.get_runs(self.runs, am, self.name)
            tests.append(self.gen_single_test(tgt, runs, asc, am.fmt(dec)))
        self.gm.alias(self.name + self.gm.test_suffix(), tests)


class MiBench:
    def __init__(self):
        self.narchs = [RV32_LINUX, X86]
        self.xarchs = [(RV32_LINUX, X86)]
        self.srcdir = path(DIR.top, "mibench")
        self.dstdir = path(DIR.build, "mibench")
        self.stack_large = ["-stack-size=131072"]    # 128K
        self.stack_huge = ["-stack-size=1048576"]    # 1M
        self.bflags = "--cc=gcc"
        self.txt = ''
        self.benchs = None


    def append(self, txt):
        self.txt = self.txt + txt


    def _gen_prologue(self):
        self.append("""\
### MIBENCH ###

all: benchs

clean:
\trm -rf {}

""".format(self.dstdir))


    def _bench(self, name, dir, ins, runs,
            dstdir=None, ctor=Bench,
            bflags=None, xflags=None,
            **kwargs):
        dstdir = dstdir if dstdir else path(self.dstdir, dir)
        bflags = cat(self.bflags, bflags)

        return ctor(
            name=name, dir=dir, ins=ins, runs=runs,
            srcdir=path(self.srcdir, dir),
            dstdir=dstdir,
            bflags=bflags, xflags=xflags,
            narchs=self.narchs,
            xarchs=self.xarchs,
            **kwargs)


    def _rijndael(self):
        dir = "security/rijndael"
        asc = mpath(self.srcdir, dir, "input_large.asc")
        enc = mpath(self.dstdir, dir, "{prefix}output_large{mode}.enc")
        dec = mpath(self.dstdir, dir, "{prefix}output_large{mode}.dec")
        key = "1234567890abcdeffedcba09876543211234567890abcdeffedcba0987654321"
        runs = Runs([
            Run([asc, enc, "e", key], "encode"),
            Run([enc, dec, "d", key], "decode")])
        runs.asc = asc
        runs.dec = dec

        return self._bench("rijndael", dir,
            ["aes.c", "aesxam.c"], runs, ctor=EncDecBench)


    def _bf(self):
        rflags = "--exp-rc=1"
        dir = "security/blowfish"
        asc = mpath(self.srcdir, dir, "input_large.asc")
        enc = mpath(self.dstdir, dir, "{prefix}output_large{mode}.enc")
        dec = mpath(self.dstdir, dir, "{prefix}output_large{mode}.asc")
        key = "1234567890abcdeffedcba0987654321"
        runs = Runs([
            Run(["e", asc, enc, key], "encode", rflags=rflags),
            Run(["d", enc, dec, key], "decode", rflags=rflags)])
        runs.asc = asc
        runs.dec = dec

        return self._bench("blowfish", dir,
                ["bf.c", "bf_skey.c",
                "bf_ecb.c", "bf_enc.c", "bf_cbc.c",
                "bf_cfb64.c", "bf_ofb64.c"],
                runs,
                ctor=EncDecBench,
                sbtflags=self.stack_large,
                mflags=[rflags])


    def _susan(self):
        # susan args
        dir = "automotive/susan"
        _in  = mpath(self.srcdir, dir, "input_large.pgm")
        out = mpath(self.dstdir, dir, "{prefix}output_large{mode}.")
        runs = Runs([
            Run([_in, out + "smoothing.pgm", "-s"], "smoothing", outidx=1),
            Run([_in, out + "edges.pgm", "-e"], "edges", outidx=1),
            Run([_in, out + "corners.pgm", "-c"], "corners", outidx=1)])

        return self._bench("susan", dir,
                ["susan.c"], runs,
                sbtflags=self.stack_huge)


    def _single_run(self, args, stdin=None, rflags=None):
        return Runs([Run(args, stdin=stdin, rflags=rflags)])


    def gen(self):
        self._gen_prologue()

        self.benchs = [
            self._bench("dijkstra", "network/dijkstra",
                ["dijkstra_large.c"],
                self._single_run(
                    [path(self.srcdir, "network/dijkstra/input.dat")])),
            self._bench("crc32", "telecomm/CRC32",
                ["crc_32.c"],
                self._single_run(
                    [path(self.srcdir, "telecomm/adpcm/data/large.pcm")])),
            self._rijndael(),
            self._bench("sha", "security/sha",
                ["sha_driver.c", "sha.c"],
                self._single_run(
                    [path(self.srcdir, "security/sha/input_large.asc")]),
                sbtflags=["-stack-size=16384"]),
            self._bench("adpcm-encode", "telecomm/adpcm/src",
                ["rawcaudio.c", "adpcm.c"],
                self._single_run([],
                    stdin=path(self.srcdir, "telecomm/adpcm/data/large.pcm"),
                    rflags="--bin"),
                dstdir=path(self.dstdir, "telecomm/adpcm/rawcaudio"),
                dbg="opt"),
            self._bench("adpcm-decode", "telecomm/adpcm/src",
                ["rawdaudio.c", "adpcm.c"],
                self._single_run([],
                    stdin=path(self.srcdir, "telecomm/adpcm/data/large.adpcm"),
                    rflags="--bin"),
                dstdir=path(self.dstdir, "telecomm/adpcm/rawdaudio")),
            self._bench("stringsearch", "office/stringsearch",
                ["bmhasrch.c", "bmhisrch.c", "bmhsrch.c", "pbmsrch_large.c"],
                self._single_run([]),
                sbtflags=self.stack_large),
            self._bf(),
            self._bench("basicmath", "automotive/basicmath",
                ["basicmath_large.c", "rad2deg.c", "cubic.c", "isqrt.c"],
                self._single_run([]),
                sbtflags=self.stack_large),
            self._bench("bitcount", "automotive/bitcount",
                ["bitcnt_1.c", "bitcnt_2.c", "bitcnt_3.c", "bitcnt_4.c",
                    "bitcnts.c", "bitfiles.c", "bitstrng.c", "bstr_i.c"],
                self._single_run(["1125000"]),
                sbtflags=self.stack_large)
                .out_filter("sed 's/Time:[^;]*; //;/^Best/d;/^Worst/d'"),
            self._bench("fft", "telecomm/FFT",
                ["main.c", "fftmisc.c", "fourierf.c"],
                Runs([
                    Run(["8", "32768"], "std"),
                    Run(["8", "32768", "-i"], "inv") ]),
                sbtflags=self.stack_large),
            self._bench("patricia", "network/patricia",
                ["patricia.c", "patricia_test.c"],
                self._single_run(
                    [path(self.srcdir, "network/patricia/large.udp")],
                    rflags="--exp-rc=1"),
                mflags=["--exp-rc=1"]),
            self._susan(),
        ]

        for bench in self.benchs:
            self.append(bench.gen())

        self._gen_epilogue()
        self._write()

    def _gen_csv_header(self):
        header = []

        ispace = ["", "", ""]
        l = ["Mibench Benchmarks", ""]
        l.extend(["Native x86"] + ispace)
        l.extend(["Globals"] + ispace)
        l.extend(["Locals"] + ispace)
        header.append(l)

        tm = "Time"
        tsd = "Time SD"
        x = "Slowdown"
        xsd = "Slowdown SD"
        item = [tm, tsd, x, xsd]

        l = ["Name", "Set"]
        l.extend(item * 3)
        header.append(l)

        return header


    def _gen_epilogue(self):
        names = [b.name for b in self.benchs]
        csv_header = self._gen_csv_header()

        self.append("""\
.PHONY: benchs
benchs: {}

.PHONY: benchs-test
benchs-test: benchs {}

.PHONY: csv-header
csv-header:
\techo "{}" > mibench.csv
\techo "{}" >> mibench.csv

.PHONY: benchs-measure
benchs-measure: benchs csv-header {}

""".format(
            " ".join(names),
            " ".join([name + "-test" for name in names]),
            ",".join(csv_header[0]),
            ",".join(csv_header[1]),
            " ".join([name + "-measure" for name in names]),
        ))


    def _write(self):
        # write txt to Makefile
        with open("Makefile", "w") as f:
            f.write(self.txt)


if __name__ == "__main__":
    mb = MiBench()
    mb.gen()


"""
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

