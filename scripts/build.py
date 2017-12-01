#!/usr/bin/python3

import argparse
import os

### config ###

# flags
MAKE_OPTS       = os.getenv("MAKE_OPTS", "-j9")
CFLAGS          = "-fno-rtti -fno-exceptions"
_O              = "-O3"

EMITLLVM        = "-emit-llvm -c {} -mllvm -disable-llvm-optzns".format(_O)
RV32_TRIPLE     = "riscv32-unknown-elf"


class Dir:
    def __init__(self):
        top             = os.environ["TOPDIR"]
        build_type      = os.getenv("BUILD_TYPE", "Debug")
        build_type_dir  = build_type.lower()
        toolchain       = top + "/toolchain"

        self.top                = top
        self.log                = top + "/junk"
        self.toolchain_release  = toolchain + "/release"
        self.toolchain_debug    = toolchain + "/debug"
        self.toolchain          = toolchain + "/" + build_type_dir
        self.remote             = top + "/remote"
        self.build              = top + "/build"
        self.patches            = top + "/patches"
        self.scripts            = top + "/scripts"
        self.submodules         = top + "/submodules"

DIR = Dir()


class Sbt:
    def __init__(self):
        self.flags = "-x"
        self.share_dir = DIR.toolchain + "/share/riscv-sbt"
        self.nat_objs = ["syscall", "counters"]
        self.nat_obj  = lambda arch, name: \
            "{}/{}-{}.o".format(self.share_dir, arch, name)

SBT = Sbt()


class Tools:
    def __init__(self):
        self.cmake      = "cmake"
        self.run_sh     = DIR.scripts + "/run.sh"
        self.log        = self.run_sh + " --log"
        self.log_clean  = "rm -f log.txt"
        self.measure    = '{}; MODES="globals locals" {} {}/measure.py'.format(
            self.log_clean, self.log, DIR.scripts)
        self.opt        = "opt"
        self.opt_flags  = _O #-stats
        self.dis        = "llvm-dis"
        self.link       = "llvm-link"

TOOLS = Tools()


class Arch:
    def __init__(self, prefix, triple, run, march, gcc_flags,
            clang_flags, sysroot, isysroot, llc_flags, as_flags,
            ld_flags):

        self.prefix = prefix
        self.triple = triple
        self.run = run
        self.march = march
        # gcc
        self.gcc = triple + "-gcc"
        self.gcc_flags = lambda opts: \
            "-static {} {} {}".format(
                ("-O0 -g" if opts.dbg else _O),
                CFLAGS,
                gcc_flags)
        # clang
        self.clang = "clang"
        self.clang_flags = "{} {}".format(CFLAGS, clang_flags)
        self.sysroot = sysroot
        self.isysroot = isysroot
        self.sysroot_flag = "-isysroot {} -isystem {}".format(
            sysroot, isysroot)
        # llc
        self.llc = "llc"
        self.llc_flags = lambda opts: \
            "-relocation-model=static " + \
            ("-O0" if opts.dbg else _O)
        # as
        self._as = triple + "-as"
        self.as_flags = as_flags
        # ld
        self.ld = triple + "-ld"
        self.ld_flags = ld_flags


PK32 = DIR.toolchain_release + "/" + RV32_TRIPLE + "/bin/pk"
RV32_SYSROOT    = DIR.toolchain_release + "/opt/riscv/" + RV32_TRIPLE
RV32_MARCH      = "riscv32"
RV32_LLC_FLAGS  = "-march=" + RV32_MARCH + " -mattr=+m"

RV32 = Arch(
        "rv32",
        RV32_TRIPLE,
        "LD_LIBRARY_PATH={}/lib spike {}".format(
            DIR.toolchain_release, PK32),
        RV32_MARCH,
        "",
        "--target=riscv32",
        RV32_SYSROOT,
        RV32_SYSROOT + "/include",
        RV32_LLC_FLAGS,
        "",
        "")


RV32_LINUX_SYSROOT  = DIR.toolchain_release + "/opt/riscv/sysroot"
RV32_LINUX_ABI      = "ilp32"

RV32_LINUX = Arch(
        "rv32",
        "riscv64-unknown-linux-gnu",
        "qemu-riscv32",
        RV32_MARCH,
        "-march=rv32g -mabi=" + RV32_LINUX_ABI,
        "--target=riscv32 -D__riscv_xlen=32",
        RV32_LINUX_SYSROOT,
        RV32_LINUX_SYSROOT + "/include",
        RV32_LLC_FLAGS,
        "-march=rv32g -mabi=" + RV32_LINUX_ABI,
        "-m elf32lriscv")


X86_MARCH = "x86"

X86 = Arch(
        "x86",
        "x86_64-linux-gnu",
        "",
        X86_MARCH,
        "-m32",
        "--target=x86_64-unknown-linux-gnu -m32",
        "/",
        "/usr/include",
        "-march=" + X86_MARCH + " -mattr=avx",
        "--32",
        "-m elf_i386")


class Opts:
    def __init__(self):
        self.dbg = False
        self.cflags = ""
        self.sflags = ""
        self.libs = ""
        self.setsysroot = True
        self.clink = True


### rules ###

# .c -> .bc
def _c2bc(arch, srcdir, dstdir, _in, out, opts):
    flags = "{} {}{} {}".format(
        arch.clang_flags,
        (arch.sysroot_flag + " " if opts.setsysroot else ""),
        opts.cflags,
        EMITLLVM)

    ipath = srcdir + "/" + _in + ".c"
    opath = dstdir + "/" + out + ".bc"

    cmd = "{} {} {} -o {}".format(
        arch.clang, flags, ipath, opath)
    print(cmd)


# *.bc -> .bc
def _lllink(arch, dir, ins, out):
    ipath = lambda mod: dir + "/" + mod + ".bc"
    opath = dir + "/" + out + ".bc"

    ipaths = ""
    for i in ins:
        ipaths = ipaths + " " + ipath(i)
    ipaths = ipaths.strip()

    cmd = "{} {} -o {}".format(
        TOOLS.link, ipaths, opath)
    print(cmd)


# .bc -> .ll
def _dis(arch, dir, mod):
    ipath = dir + "/" + mod + ".bc"
    opath = dir + "/" + mod + ".ll"

    cmd = "{} {} -o {}".format(
        TOOLS.dis, ipath, opath)
    print(cmd)


# opt
def _opt(arch, dir, _in, out):
    ipath = dir + "/" + _in + ".bc"
    opath = dir + "/" + out + ".bc"

    cmd = "{} {} {} -o {}".format(
        TOOLS.opt, TOOLS.opt_flags, ipath, opath)
    print(cmd)


# *.c -> *.bc -> .bc
# (C2LBC: C to linked BC)
def _c2lbc(arch, srcdir, dstdir, ins, out, opts):
    if len(ins) > 1:
        ins2 = []
        for c in ins:
            o = c + "_bc1"
            ins2.append(o)

            _c2bc(arch, srcdir, dstdir, c, o, opts)
            _dis(arch, dstdir, o)

        _lllink(arch, dstdir, ins2, out)
    else:
        _c2bc(arch, srcdir, dstdir, ins[0], out, opts)
    _dis(arch, dstdir, out)


# .bc -> .s
def _bc2s(arch, dir, _in, out, opts):
    ipath = dir + "/" + _in + ".bc"
    opath = dir + "/" + out + ".s"

    flags = "{} {}".format(
        arch.llc_flags, opts.sflags)
    flags = flags.strip()

    cmd = "{} {} {} -o {}".format(
        arch.llc, flags, ipath, opath)
    print(cmd)
    if arch == RV32:
        cmd = 'sed -i "1i.option norelax" ' + opath
        print(cmd)


"""
# opt; dis; .bc -> .s
def _opt1NBC2S
$(eval OPT1NBC2S_DSTDIR = $(3))
$(eval OPT1NBC2S_OUT = $(5))
$(eval OPT1NBC2S_FLAGS = $(6))

$(if $(DEBUG_RULES),$(warning "OPT1NBC2S(DSTDIR=$(OPT1NBC2S_DSTDIR),\
OUT=$(OPT1NBC2S_OUT))"),)

$(call _OPT,$(1),$(OPT1NBC2S_DSTDIR),$(OPT1NBC2S_OUT),$(OPT1NBC2S_OUT).opt)
$(call _DIS,$(1),$(OPT1NBC2S_DSTDIR),$(OPT1NBC2S_OUT).opt)

$(call _BC2S,$(1),$(OPT1NBC2S_DSTDIR),$(OPT1NBC2S_OUT).opt,\
$(OPT1NBC2S_OUT),$(OPT1NBC2S_FLAGS))
endef


"""


# *.c -> .s
def _c2s(arch, srcdir, dstdir, ins, out, opts):
    _c2lbc(arch, srcdir, dstdir, ins, out, opts)

    opt1 = out + ".opt"
    opt2 = out + ".opt2"

    # opt; dis; opt; dis; .bc -> .s
    _opt(arch, dstdir, out, opt1)
    _dis(arch, dstdir, opt1)
    _opt(arch, dstdir, opt1, opt2)
    _dis(arch, dstdir, opt2)
    _bc2s(arch, dstdir, opt2, out, opts)


# .s -> .o
def _s2o(arch, srcdir, dstdir, _in, out, opts):
    ipath = srcdir + "/" + _in + ".s"
    opath = dstdir + "/" + out + ".o"

    flags = "{} {}".format(arch.as_flags, opts.sflags)
    flags = flags.strip()

    cmd = "{} {} -c {} -o {}".format(
        arch._as, flags, ipath, opath)
    print(cmd)


# *.s -> *.o
def _s2os():

$(if $(subst 1,,$(words $(S2OS_INS))),\
$(foreach in,$(S2OS_INS),\
$(call _S2O,$(1),$(S2OS_SRCDIR),$(S2OS_DSTDIR),$(in),$(in),$(S2OS_FLAGS))),\
$(call _S2O,$(1),$(S2OS_SRCDIR),$(S2OS_DSTDIR),$(S2OS_INS),$(S2OS_OUT),$(S2OS_FLAGS)))
endef


# *.o -> bin
def _link(arch, srcdir, dstdir, ins, out, opts):
    libs = opts.libs
    if opts.clink:
        tool = arch.gcc
        flags = arch.gcc_flags(opts)
        libs = libs + " -lm"
    else:
        tool = arch.ld
        flags = arch.ld_flags
    flags = flags + " " + opts.ldflags
    flags = flags.strip()
    libs = libs.strip()

    ipaths = [srcdir + "/" + i + ".o" for i in ins]
    opath = dstdir + "/" + out
    cmd = "{} {} {} -o {} {}".format(
        tool, flags, " ".join(ipaths), out, libs)


# *.s -> bin
def _snlink(arch, srcdir, dstdir, ins, out, opts):
    if len(ins) == 1:
        _s2o(arch, srcdir, dstdir, ins[0], out, opts)
        _link(arch, dstdir, dstdir, out, out, opts)
    else:
        for i in ins:
            _s2o(arch, srcdir, dstdir, i, i, opts)
        _link(arch, dstdir, dstdir, ins, out, opts)


# *.c -> bin
def _cnlink(arch, srcdir, dstdir, ins, out, opts):
    _c2s(arch, srcdir, dstdir, ins, out, opts)
    _s2o(arch, dstdir, dstdir, out, out)
    _link(arch, dstdir, dstdir, out, out, opts)


# clean
def _clean(dir, mod, clean_s):
    if os.path.isdir(dir) and os.path.exists(dir):
        suf =

		cd $(CLEAN_DIR) && \
		rm -f $(CLEAN_MOD) \
		$(CLEAN_MOD).o \
		$(if $(CLEAN_S),$(CLEAN_MOD).s,) \
		$(CLEAN_MOD).bc $(CLEAN_MOD).ll \
		$(CLEAN_MOD).opt.bc $(CLEAN_MOD).opt.ll \
		$(CLEAN_MOD).opt2.bc $(CLEAN_MOD).opt2.ll \
		$(CLEAN_MOD).out; \
	fi

endef


# run
define _RUN
$(eval RUN_DIR = $(2))
$(eval RUN_PROG = $(3))
$(eval RUN_FLAGS = $(4))
$(eval RUN_ARGS = $(5))

$(eval RUN_SAVE_OUT = $(findstring $(SAVE_OUT),$(RUN_FLAGS)))
$(eval RUN_NOTEE = $(findstring $(NOTEE),$(RUN_FLAGS)))

$(if $(DEBUG_RULES),$(warning "RUN(DIR=$(RUN_DIR),\
PROG=$(RUN_PROG),\
FLAGS=$(RUN_FLAGS),\
ARGS=$(RUN_ARGS))"),)

$(RUN_PROG)-run: $(RUN_DIR)$(RUN_PROG)
	@echo $$@:
	$(if $(RUN_SAVE_OUT),$(RUN_SH) -o $(RUN_DIR)$(RUN_PROG).out,) \
		$(subst $(NOTEE),--no-tee,$(RUN_NOTEE)) \
		$($(1)_RUN) $(RUN_DIR)$(RUN_PROG) $(RUN_ARGS)

endef


# alias
define _ALIAS
$(eval ALIAS_DIR = $(1))
$(eval ALIAS_BIN = $(2))

$(if $(DEBUG_RULES),$(warning "ALIAS(DIR=$(ALIAS_DIR),\
BIN=$(ALIAS_BIN))"),)

ifneq ($(ALIAS_DIR),./)
.PHONY: $(ALIAS_BIN)
$(ALIAS_BIN): $(ALIAS_DIR) $(ALIAS_DIR)$(ALIAS_BIN)
endif

endef


# *.c/s -> bin
define BUILD
$(eval BUILD_SRCDIR = $(2))
$(eval BUILD_DSTDIR = $(3))
$(eval BUILD_INS = $(4))
$(eval BUILD_OUT = $(5))
$(eval BUILD_LIBS = $(6))
$(eval BUILD_FLAGS = $(7))
$(eval BUILD_ASM = $(8))
$(eval BUILD_C = $(9))
$(eval BUILD_RUNARGS = $(10))

$(if $(DEBUG_RULES),$(warning "BUILD($(1),\
SRCDIR=$(BUILD_SRCDIR),\
DSTDIR=$(BUILD_DSTDIR),\
INS=$(BUILD_INS),\
OUT=$(BUILD_OUT),\
LIBS=$(BUILD_LIBS),\
FLAGS=$(BUILD_FLAGS),\
ASM=$(BUILD_ASM),\
C=$(BUILD_C),\
RUNARGS=$(BUILD_RUNARGS))"),)

$(eval BUILD_RUNFLAGS = $(BUILD_FLAGS) $(SAVE_OUT))
$(eval BUILD_FLAGS = $(subst $(NOTEE),,$(BUILD_FLAGS)))

$(call $(if $(BUILD_ASM),_SNLINK,_CNLINK)\
,$(1),$(BUILD_SRCDIR),$(BUILD_DSTDIR),$(BUILD_INS),\
$(BUILD_OUT),$(BUILD_LIBS),$(BUILD_FLAGS),$(BUILD_C))

$(call _CLEAN,$(BUILD_DSTDIR),$(BUILD_OUT),$(if $(BUILD_C),,clean.s))
$(call _RUN,$(1),$(BUILD_DSTDIR),$(BUILD_OUT),$(BUILD_RUNFLAGS),$(BUILD_RUNARGS))
$(call _ALIAS,$(BUILD_DSTDIR),$(BUILD_OUT))
endef


# .o -> .bc
define _TRANSLATE_OBJ
$(eval TO_DIR = $(2))
$(eval TO_IN = $(3))
$(eval TO_OUT = $(4))
$(eval TO_FLAGS = $(5))

$(if $(DEBUG_RULES),$(warning "TRANSLATE_OBJ($(1),\
DIR=$(TO_DIR),\
IN=$(TO_IN),\
OUT=$(TO_OUT),\
FLAGS=$(TO_FLAGS))"),)

$(TO_DIR)$(TO_OUT).bc: $(TO_DIR)$(TO_IN).o
	@echo $$@: $$<
	riscv-sbt $(SBTFLAGS) $(TO_FLAGS) -o $$@ $$^ >$(TOPDIR)/junk/$(TO_OUT).log 2>&1

endef


# .o -> bin
define TRANSLATE
$(eval TRANSLATE_SRCDIR = $(2))
$(eval TRANSLATE_DSTDIR = $(3))
$(eval TRANSLATE_IN = $(4))
$(eval TRANSLATE_OUT = $(5))
$(eval TRANSLATE_LIBS = $(6))
$(eval TRANSLATE_FLAGS = $(7))
$(eval TRANSLATE_C = $(8))
$(eval TRANSLATE_RUNARGS = $(9))

$(eval TRANSLATE_NAT_OBJS = $(foreach obj,$(SBT_NAT_OBJS),$($(1)_$(obj)_O)))

$(if $(DEBUG_RULES),$(warning "TRANSLATE($(1),\
SRCDIR=$(TRANSLATE_SRCDIR),\
DSTDIR=$(TRANSLATE_DSTDIR),\
IN=$(TRANSLATE_IN),\
OUT=$(TRANSLATE_OUT),\
LIBS=$(TRANSLATE_LIBS),\
FLAGS=$(TRANSLATE_FLAGS),\
C=$(TRANSLATE_C),\
RUNARGS=$(TRANSLATE_RUNARGS))"),)

$(eval TRANSLATE_DBG = $(findstring $(DEBUG),$(TRANSLATE_FLAGS)))
$(eval TRANSLATE_TFLAGS = $(subst $(DEBUG),,$(TRANSLATE_FLAGS)))
$(eval TRANSLATE_TFLAGS = $(subst $(NOTEE),,$(TRANSLATE_FLAGS)))
$(eval TRANSLATE_FLAGS = $(patsubst -regs=%,,$(TRANSLATE_FLAGS)))

$(call _TRANSLATE_OBJ,$(1),$(TRANSLATE_DSTDIR),$(TRANSLATE_IN),\
$(TRANSLATE_OUT),$(TRANSLATE_TFLAGS) $(if $(TRANSLATE_C),,-dont-use-libc))

$(call _DIS,$(1),$(TRANSLATE_DSTDIR),$(TRANSLATE_OUT))

$(if $(TRANSLATE_DBG),\
$(call _BC2S,$(1),$(TRANSLATE_DSTDIR),$(TRANSLATE_OUT),$(TRANSLATE_OUT),\
$(DEBUG)),\
$(call _OPT1NBC2S,$(1),$(TRANSLATE_DSTDIR),$(TRANSLATE_DSTDIR),\
$(TRANSLATE_OUT),$(TRANSLATE_OUT),$(TRANSLATE_FLAGS)))

$(call BUILD,$(1),$(TRANSLATE_DSTDIR),$(TRANSLATE_DSTDIR),\
$(TRANSLATE_OUT),$(TRANSLATE_OUT),\
$(TRANSLATE_LIBS) $(TRANSLATE_NAT_OBJS),\
$(if $(TRANSLATE_DBG),$(DEBUG),) $(TRANSLATE_FLAGS),\
$(ASM),$(TRANSLATE_C),$(TRANSLATE_RUNARGS))
endef


### sbt test ###

MODES        := globals locals
# NARCHS = native archs to build bins to
TEST_NARCHS  := RV32 X86
UTEST_NARCHS := RV32
ADD_ASM_SRC_PREFIX := 1


define _NBUILD
$(eval N_ARCH = $(1))
$(eval N_SRCDIR = $(2))
$(eval N_DSTDIR = $(3))
$(eval N_INS = $(4))
$(eval N_OUT = $(5))
$(eval N_LIBS = $(6))
$(eval N_FLAGS = $(7))
$(eval N_ASM = $(8))
$(eval N_C = $(9))
$(eval N_ARGS = $(10))

$(eval N_PREFIX = $($(N_ARCH)_PREFIX))
$(eval N_INS = $(if $(and $(ADD_ASM_SRC_PREFIX),$(N_ASM)),\
$(addprefix $(N_PREFIX)-,$(N_INS)),$(N_INS)))
$(eval N_OUT = $(N_PREFIX)-$(N_OUT))

$(if $(DEBUG_RULES),$(warning "NBUILD($(1),\
SRCDIR=$(N_DSTDIR),\
DSTDIR=$(N_DSTDIR),\
INS=$(N_INS),\
OUT=$(N_OUT),\
LIBS=$(N_LIBS),\
FLAGS=$(N_FLAGS),\
ASM=$(N_ASM),\
C=$(N_C),\
ARGS=$(N_ARGS))"),)

# native bins
$(call BUILD,$(N_ARCH),$(N_SRCDIR),$(N_DSTDIR),$(N_INS),$(N_OUT),\
$(N_LIBS),$(N_FLAGS),$(N_ASM),$(N_C),$(N_ARGS))

endef


define _SBT_TEST1
$(eval TEST1_DSTARCH = $(1))
$(eval TEST1_DSTDIR = $(2))
$(eval TEST1_NAME = $(3))
$(eval TEST1_MODE = $(4))
$(eval TEST1_LIBS = $(5))
$(eval TEST1_FLAGS = $(6))
$(eval TEST1_C = $(7))
$(eval TEST1_ARGS = $(8))

$(eval TEST1_TGT = $(TEST1_NAME)-$(TEST1_MODE))
$(eval TEST1_IN = $(RV32_PREFIX)-$(TEST1_NAME))
$(eval TEST1_OUT = $(RV32_PREFIX)-$($(TEST1_DSTARCH)_PREFIX)-$(TEST1_TGT))
$(eval TEST1_NAT = $($(TEST1_DSTARCH)_PREFIX)-$(TEST1_NAME))

$(if $(DEBUG_RULES),$(warning "SBT_TEST1($(1),\
DSTDIR=$(TEST1_DSTDIR),\
NAME=$(TEST1_NAME),\
MODE=$(TEST1_MODE),\
LIBS=$(TEST1_LIBS),\
FLAGS=$(TEST1_FLAGS),\
C=$(TEST1_C),\
ARGS=$(TEST1_ARGS))"),)

$(call TRANSLATE,$(TEST1_DSTARCH),$(TEST1_DSTDIR),$(TEST1_DSTDIR),\
$(TEST1_IN),$(TEST1_OUT),$(TEST1_LIBS),$(TEST1_FLAGS) -regs=$(TEST1_MODE),\
$(TEST1_C),$(TEST1_ARGS))

.PHONY: $(TEST1_TGT)
$(TEST1_TGT): $(TEST1_DSTDIR)$(TEST1_OUT)

$(TEST1_TGT)-test: $(TEST1_TGT) $(TEST1_OUT)-run
	diff $(TEST1_DSTDIR)$(TEST1_IN).out $(TEST1_DSTDIR)$(TEST1_OUT).out

endef


define SBT_TEST
$(eval TEST_ARCHS = $(1))
$(eval TEST_SRCDIR = $(2))
$(eval TEST_DSTDIR = $(3))
$(eval TEST_INS = $(4))
$(eval TEST_OUT = $(5))
$(eval TEST_LIBS = $(6))
$(eval TEST_FLAGS = $(7))
$(eval TEST_ASM = $(8))
$(eval TEST_C = $(9))
$(eval TEST_ARGS = $(10))

$(if $(DEBUG_RULES),$(warning "SBT_TEST($(1),\
SRCDIR=$(TEST_SRCDIR),\
DSTDIR=$(TEST_DSTDIR),\
INS=$(TEST_INS),\
OUT=$(TEST_OUT),\
LIBS=$(TEST_LIBS),\
FLAGS=$(TEST_FLAGS),\
ASM=$(TEST_ASM),\
C=$(TEST_C),\
ARGS=$(TEST_ARGS))"),)

# native bins
$(eval TEST_NFLAGS = $(subst $(DEBUG),,$(TEST_FLAGS)))

$(foreach ARCH,$(TEST_ARCHS),\
$(call _NBUILD,$(ARCH),$(TEST_SRCDIR),$(TEST_DSTDIR),\
$(TEST_INS),$(TEST_OUT),$(TEST_LIBS),\
$(if $(findstring RV32_LINUX,$(ARCH)),$(NOSYSROOT) $(X86_SYSROOT_FLAG),) $(TEST_NFLAGS),\
$(TEST_ASM),$(TEST_C),$(TEST_ARGS)))

# translated bins
$(foreach mode,$(MODES),\
$(call _SBT_TEST1,X86,$(TEST_DSTDIR),$(TEST_OUT),$(mode),\
$(TEST_LIBS),$(TEST_FLAGS),$(TEST_C),$(TEST_ARGS)))

.PHONY: $(TEST_OUT)
$(TEST_OUT): $(foreach ARCH,$(TEST_ARCHS),$($(ARCH)_PREFIX)-$(TEST_OUT)) \
             $(foreach mode,$(MODES),$(TEST_OUT)-$(mode))

$(TEST_OUT)-test: $(foreach ARCH,$(TEST_ARCHS),$($(ARCH)_PREFIX)-$(TEST_OUT)-run) \
                  $(foreach mode,$(MODES),$(TEST_OUT)-$(mode)-test)

endef

# .o -> .bc
def translate_obj(dir, in, out, flags):
    print("translate_obj(dir={0}, in={1}, out={2}, flags={3})".format(dir, in, out, flags))

    opath = dir + out + ".bc"
    ipath = dir + in + ".o"
    log = LOG_DIR + "/" + out + ".log"

    print("{0}: {1}".format(opath, ipath))
    cmd = "riscv-sbt {0} {1} -o {2} {3} >{4} 2>&1".format(
        SBTFLAGS, flags, opath, ipath, log)


# .o -> bin
def translate(arch, srcdir, dstdir, in, out, libs, flags, c, runargs='',
        dbg=False):
    print(("translate(arch={0}, srcdir={1}, dstdir={2}, in={3}, out={4}, " + \
        "libs={5}, flags={6}, c={7}, runargs={8})").format(
        arch, srcdir, dstdir, in, out, libs, flags, c, runargs))

    nat_objs = 

    translate_obj(dstdir, in, out, flags, (c? "-dont-use-libc" : ""))
    dis(arch, dstdir, out)

$(eval TRANSLATE_NAT_OBJS = $(foreach obj,$(SBT_NAT_OBJS),$($(1)_$(obj)_O)))

$(call _DIS,$(1),$(TRANSLATE_DSTDIR),$(TRANSLATE_OUT))

$(if $(TRANSLATE_DBG),\
$(call _BC2S,$(1),$(TRANSLATE_DSTDIR),$(TRANSLATE_OUT),$(TRANSLATE_OUT),\
$(DEBUG)),\
$(call _OPT1NBC2S,$(1),$(TRANSLATE_DSTDIR),$(TRANSLATE_DSTDIR),\
$(TRANSLATE_OUT),$(TRANSLATE_OUT),$(TRANSLATE_FLAGS)))

$(call BUILD,$(1),$(TRANSLATE_DSTDIR),$(TRANSLATE_DSTDIR),\
$(TRANSLATE_OUT),$(TRANSLATE_OUT),\
$(TRANSLATE_LIBS) $(TRANSLATE_NAT_OBJS),\
$(if $(TRANSLATE_DBG),$(DEBUG),) $(TRANSLATE_FLAGS),\
$(ASM),$(TRANSLATE_C),$(TRANSLATE_RUNARGS))
endef


def main():
    _c2lbc(RV32, "srcdir", "dstdir", ["hello1", "hello2"], "hello", "", "-DROWS=3")

main()
