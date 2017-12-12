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
    def __init__(self, name, dir, mods, args):
        self.name = name
        self.dir = dir
        self.mods = mods
        self.args = args


if __name__ == "__main__":
    srcdir = path(DIR.top, "mibench")
    dstdir = path(DIR.build, "mibench")

    benchs = [
        Bench("dijkstra", "network/dijkstra",
            ["dijkstra_large.c"],
            path(srcdir, "network/dijkstra/input.dat")),
        Bench("crc32", "telecomm/CRC32",
            ["crc_32.c"],
            path(srcdir, "telecomm/adpcm/data/large.pcm")),
    ]

    txt = ''
    narchs = ["rv32", "x86"]
    xarchs = ["rv32-x86"]
    xflags = None
    bflags = None
    rflags = "-o {}.out"
    for bench in benchs:
        name = bench.name
        dir = bench.dir
        src = bench.mods[0]
        txt = txt + do_mod(narchs, xarchs, name, src,
                path(srcdir, dir), path(dstdir, dir),
                xflags, bflags, rflags)

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

## 06- RIJNDAEL
# rv32: OK

RIJNDAEL_NAME   := rijndael
RIJNDAEL_DIR    := security/rijndael
RIJNDAEL_MODS   := aes aesxam
RIJNDAEL_ARGS   := notests

## 07- BLOWFISH
# rv32: wrong results

BLOWFISH_NAME   := blowfish
BLOWFISH_DIR    := security/blowfish
BLOWFISH_MODS   := bf bf_skey bf_ecb bf_enc bf_cbc bf_cfb64 bf_ofb64
BLOWFISH_ARGS   := notests

## 08- SHA
# rv32: OK

SHA_NAME        := sha
SHA_DIR         := security/sha
SHA_MODS        := sha_driver sha
SHA_ARGS        := $(MIBENCH)/$(SHA_DIR)/input_large.asc

## 09- RAWCAUDIO
# rv32: OK

RAWCAUDIO_NAME  := rawcaudio
RAWCAUDIO_DIR   := telecomm/adpcm
RAWCAUDIO_MODS  := rawcaudio adpcm
RAWCAUDIO_ARGS  := < $(MIBENCH)/$(RAWCAUDIO_DIR)/data/large.pcm
RAWCAUDIO_SRC_DIR_SUFFIX := /src
RAWCAUDIO_OUT_DIR_SUFFIX := /rawcaudio
RAWCAUDIO_NO_TEE := 1

## 10- RAWDAUDIO
# rv32: OK

RAWDAUDIO_NAME  := rawdaudio
RAWDAUDIO_DIR   := telecomm/adpcm
RAWDAUDIO_MODS  := rawdaudio adpcm
RAWDAUDIO_ARGS  := < $(MIBENCH)/$(RAWDAUDIO_DIR)/data/large.adpcm
RAWDAUDIO_SRC_DIR_SUFFIX := /src
RAWDAUDIO_OUT_DIR_SUFFIX := /rawdaudio
RAWDAUDIO_NO_TEE := 1

## 12- FFT
# rv32: OK (soft-float)

FFT_NAME        := fft
FFT_DIR         := telecomm/FFT
FFT_MODS        := main fftmisc fourierf
FFT_ARGS        := notests

## 13- STRINGSEARCH
# rv32: OK

STRINGSEARCH_NAME := stringsearch
STRINGSEARCH_BIN  := search_large
STRINGSEARCH_DIR  := office/stringsearch
STRINGSEARCH_MODS := bmhasrch bmhisrch bmhsrch pbmsrch_large
STRINGSEARCH_ARGS :=

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


### enabled benchmarks

BENCHS := \
    DIJKSTRA \
    CRC32 \
    RIJNDAEL \


    #BASICMATH \
    BITCOUNT \
    SUSAN \
    PATRICIA \
    BLOWFISH \
    SHA \
    RAWCAUDIO \
    RAWDAUDIO \
    FFT \
    STRINGSEARCH \
    LAME


### rules

all: $(foreach bench,$(BENCHS),$($(bench)_NAME))

clean:
	rm -rf $(MIBENCH_DSTDIR)

include $(TOPDIR)/make/rules.mk




# main mibench macro
define MBENCH
$(eval MBENCH_BENCH = $(1))
$(eval MBENCH_DIR = $($(1)_DIR))
$(eval MBENCH_SRCDIR = $(MIBENCH_SRCDIR)/$(MBENCH_DIR))
$(eval MBENCH_DSTDIR = $(MIBENCH_DSTDIR)/$(MBENCH_DIR))
$(eval MBENCH_NAME = $($(1)_NAME))
$(eval MBENCH_MODS = $($(1)_MODS))
$(eval MBENCH_ARGS = $($(1)_ARGS))

$(MBENCH_NAME)-clean:
	rm -rf $(MBENCH_DSTDIR)

$(eval GFLAGS = )
$(eval LFLAGS = )
$(eval RFLAGS = $(MBENCH_ARGS) --save-output)
$(eval TLFLAGS = )

# NOTE: if libc translation starts failing it may be
#       necessary to build for RV32 with X86 sysroot

$(foreach ARCH,$(ARCHS),\
$(call TGT,$(ARCH),$(MBENCH_SRCDIR),$(MBENCH_DSTDIR),\
$(MBENCH_MODS),$(subst -linux,,$(ARCH))-$(MBENCH_NAME),\
$(GFLAGS),$(LFLAGS),$(RFLAGS)))

$(call TRANSLATE,x86,$(MBENCH_DSTDIR),\
rv32-$(MBENCH_NAME),rv32-x86-$(MBENCH_NAME),\
$(GFLAGS),$(LFLAGS),$(RFLAGS),$(TLFLAGS))

$(if $(findstring notests,$(MBENCH_ARGS)),,$(call MBENCH_TEST))
endef


# call MBENCH for each BENCH

$(foreach bench,$(BENCHS),\
$(eval $(call MBENCH,$(bench))))

.PHONY: test
test: $(foreach bench,$(BENCHS),$($(bench)_NAME)-test)

.PHONY: measure
measure: $(foreach bench,$(BENCHS),$($(bench)_NAME)-measure)


### RIJNDAEL ###


RIJNDAEL_SRCDIR := $(MIBENCH_SRCDIR)/$(RIJNDAEL_DIR)
RIJNDAEL_DSTDIR := $(MIBENCH_DSTDIR)/$(RIJNDAEL_DIR)

$(eval $(call MBENCH_RUN,RIJNDAEL,encode,\
$(RIJNDAEL_SRCDIR)/input_large.asc \
$(RIJNDAEL_DSTDIR)/output_large.enc e \
1234567890abcdeffedcba09876543211234567890abcdeffedcba0987654321,\
output_large.enc))

$(eval $(call MBENCH_RUN,RIJNDAEL,decode,\
$(RIJNDAEL_DSTDIR)/output_large.enc \
$(RIJNDAEL_DSTDIR)/output_large.dec d \
1234567890abcdeffedcba09876543211234567890abcdeffedcba0987654321,\
output_large.dec))

.PHONY: $(RIJNDAEL_NAME)-test
$(RIJNDAEL_NAME)-test: $(foreach rn,encode decode,$(RIJNDAEL_NAME)-$(rn)-run)
	$(call TT,$(RIJNDAEL_DSTDIR),)
"""

