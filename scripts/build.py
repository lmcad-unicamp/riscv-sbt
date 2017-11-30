#!/usr/bin/python3

import argparse
import os

### config ###

# flags
MAKE_OPTS           = os.getenv("MAKE_OPTS", "-j9")
CFLAGS              = "-fno-rtti -fno-exceptions"
_O                  = "-O3"
EMITLLVM            = "-emit-llvm -c " + _O + " -mllvm -disable-llvm-optzns"

RV32_TRIPLE         = "riscv32-unknown-elf"

class Dir:
    def __init__(self):
        top = os.environ["TOPDIR"]
        build_type  = os.getenv("BUILD_TYPE", "Debug")
        build_type_dir = build_type.lower()
        toolchain = top + "/toolchain"

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
            self.share_dir + "/" + arch + "-" + name + ".o"

SBT = Sbt()


class Tools:
    def __init__(self):
        self.cmake      = "cmake"
        self.run_sh     = DIR.scripts + "/run.sh"
        self.log        = self.run_sh + " --log"
        self.log_clean  = "rm -f log.txt"
        self.measure    = self.log_clean + '; MODES="globals locals" ' + \
                          self.log + DIR.scripts + "/measure.py"
        self.opt        = "opt"
        self.opt_flags  = _O #-stats
        self.dis        = "llvm-dis"
        self.link       = "llvm-link"

TOOLS = Tools()


class Arch:
    def __init__(self, prefix, triple, run, march, gcc_flags,
            clang_flags, sysroot, isysroot, llc_flags, as_flags,
            ld_flags, dbg=False):

        self.prefix = prefix
        self.triple = triple
        self.run = run
        self.march = march
        # gcc
        self.gcc = triple + "-gcc"
        self.gcc_flags = "-static " + ("-O0 -g" if dbg else _O) + " " + \
            CFLAGS + " " + gcc_flags
        # clang
        self.clang = "clang"
        self.clang_flags = CFLAGS + " " + clang_flags
        self.sysroot = sysroot
        self.isysroot = isysroot
        self.sysroot_flag = "-isysroot " + sysroot + " -isystem " + isysroot
        # llc
        self.llc = "llc"
        if dbg:
            self.llc_flags = "-relocation-model=static -O0"
        else:
            self.llc_flags = "-relocation-model=static " + _O #-stats"
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
        "LD_LIBRARY_PATH=" + DIR.toolchain_release +
            "/lib spike " + PK32,
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


### rules ###

DEBUG_RULES         = False

# .c -> .bc
def _c2bc(arch, srcdir, dstdir, _in, out, flags, setsysroot=True):
    if setsysroot:
        flags = flags + arch.sysroot_flag

    opath = dstdir + "/" + out + ".bc"
    ipath = srcdir + "/" + _in + ".c"

    cmd = "{0} {1} {2} {3} -o {4}".format(
        arch.clang, arch.clang_flags, EMITLLVM, ipath, opath)
    print(cmd)


# *.bc -> .bc
def _lllink(arch, dir, ins, out):
    ipath = lambda mod: dir + "/" + mod + ".bc"
    opath = dir + "/" + out + ".bc"

    ipaths = ""
    for i in ins:
        ipaths = ipaths + " " + ipath(i)
    ipaths = ipaths.strip()

    cmd = "{0} {1} -o {2}".format(
        TOOLS.link, ipaths, opath)
    print(cmd)


# .bc -> .ll
def _dis(arch, dir, mod):
    ipath = dir + "/" + mod + ".bc"
    opath = dir + "/" + mod + ".ll"

    cmd = "{0} {1} -o {2}".format(
        TOOLS.dis, ipath, opath)
    print(cmd)


# opt
def _opt(arch, dir, _in, out):
    ipath = dir + "/" + _in + ".bc"
    opath = dir + "/" + out + ".bc"

    cmd = "{0} {1} {2} -o {3}".format(
        TOOLS.opt, TOOLS.opt_flags, ipath, opath)
    print(cmd)


# path -> dir/prefix-file
# 1: prefix
# 2: path
#define _ADD_PREFIX
#$(dir $(2))$(1)-$(notdir $(2))
#endef


# *.c -> *.bc -> .bc
# (C2LBC: C to linked BC)
def _c2lbc(arch, srcdir, dstdir, ins, out, libs, flags):
    if len(ins) > 1:
        ins2 = []
        for c in ins:
            o = c + "_bc1"
            ins2.append(o)

            _c2bc(arch, srcdir, dstdir, c, o, flags)
            _dis(arch, dstdir, o)

        _lllink(arch, dstdir, ins2, out)
    else:
        _c2bc(arch, srcdir, dstdir, ins[0], out, flags)
    _dis(arch, dstdir, out)


"""
# .bc -> .s
define _BC2S
$(eval BC2S_DIR = $(2))
$(eval BC2S_IN = $(3))
$(eval BC2S_OUT = $(4))
$(eval BC2S_FLAGS = $(5))

$(if $(DEBUG_RULES),$(warning "BC2S($(1),\
DIR=$(BC2S_DIR),\
IN=$(BC2S_IN),\
OUT=$(BC2S_OUT),\
FLAGS=$(BC2S_FLAGS))"),)

$(eval BC2S_DEBUG = $(findstring $(DEBUG),$(BC2S_FLAGS)))
$(eval BC2S_FLAGS = $(subst $(DEBUG),,$(BC2S_FLAGS)))
$(eval BC2S_FLAGS = $(if $(BC2S_DEBUG),$(LLC_DBG_FLAGS),$(LLC_FLAGS)))

$(BC2S_DIR)$(BC2S_OUT).s: $(BC2S_DIR)$(BC2S_IN).bc $(BC2S_DIR)$(BC2S_IN).ll
	@echo $$@: $$<
	$(LLC) $(BC2S_FLAGS) $($(1)_LLC_FLAGS) $$< -o $$@
ifeq ($(1),RV32)
	sed -i "1i.option norelax" $$@
endif

endef


# opt; dis; .bc -> .s
define _OPT1NBC2S
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


# opt; dis; opt; dis; .bc -> .s
define _OPT2NBC2S
$(eval OPT2NBC2S_DSTDIR = $(3))
$(eval OPT2NBC2S_OUT = $(5))
$(eval OPT2NBC2S_FLAGS = $(6))

$(if $(DEBUG_RULES),$(warning "OPT2NBC2S(DSTDIR=$(OPT2NBC2S_DSTDIR),\
OUT=$(OPT2NBC2S_OUT))"),)

$(call _OPT,$(1),$(OPT2NBC2S_DSTDIR),$(OPT2NBC2S_OUT),$(OPT2NBC2S_OUT).opt)
$(call _DIS,$(1),$(OPT2NBC2S_DSTDIR),$(OPT2NBC2S_OUT).opt)
$(call _OPT,$(1),$(OPT2NBC2S_DSTDIR),$(OPT2NBC2S_OUT).opt,$(OPT2NBC2S_OUT).opt2)
$(call _DIS,$(1),$(OPT2NBC2S_DSTDIR),$(OPT2NBC2S_OUT).opt2)
$(call _BC2S,$(1),$(OPT2NBC2S_DSTDIR),$(OPT2NBC2S_OUT).opt2,\
$(OPT2NBC2S_OUT),$(OPT2NBC2S_FLAGS))
endef


# *.c -> .s
define _C2S
$(eval C2S_SRCDIR = $(2))
$(eval C2S_DSTDIR = $(3))
$(eval C2S_INS = $(4))
$(eval C2S_OUT = $(5))
$(eval C2S_LIBS = $(6))
$(eval C2S_FLAGS = $(7))

$(if $(DEBUG_RULES),$(warning "C2S(SRCDIR=$(C2S_SRCDIR),\
DSTDIR=$(C2S_DSTDIR),\
INS=$(C2S_INS),\
OUT=$(C2S_OUT),\
LIBS=$(C2S_LIBS),\
FLAGS=$(C2S_FLAGS))"),)

$(call _C2LBC,$(1),$(C2S_SRCDIR),$(C2S_DSTDIR),\
$(C2S_INS),$(C2S_OUT),$(C2S_LIBS),$(C2S_FLAGS))
$(call _OPT2NBC2S,$(1),$(C2S_SRCDIR),$(C2S_DSTDIR),\
$(C2S_INS),$(C2S_OUT),$(C2S_FLAGS))
endef


# .s -> .o
define _S2O
$(eval S2O_SRCDIR = $(2))
$(eval S2O_DSTDIR = $(3))
$(eval S2O_IN = $(4))
$(eval S2O_OUT = $(5))
$(eval S2O_FLAGS = $(6))

$(if $(DEBUG_RULES),$(warning "S2O($(1),\
SRCDIR=$(S2O_SRCDIR),\
DSTDIR=$(S2O_DSTDIR),\
IN=$(S2O_IN),\
OUT=$(S2O_OUT),\
FLAGS=$(S2O_FLAGS))"),)

$(eval S2O_DBG = $(findstring $(DEBUG),$(S2O_FLAGS)))
$(eval S2O_FLAGS = $(subst $(DEBUG),-g,$(S2O_FLAGS)))

$(S2O_DSTDIR)$(S2O_OUT).o: $(S2O_SRCDIR)$(S2O_IN).s \
							| $(if $(subst ./,,$(S2O_DSTDIR)),$(S2O_DSTDIR),)
	@echo $$@: $$<
	$$($(1)_AS) $(S2O_FLAGS) -o $$@ -c $$<

endef


# *.s -> *.o
define _S2OS
$(eval S2OS_SRCDIR = $(2))
$(eval S2OS_DSTDIR = $(3))
$(eval S2OS_INS = $(4))
$(eval S2OS_OUT = $(5))
$(eval S2OS_FLAGS = $(6))

$(if $(DEBUG_RULES),$(warning "S2OS($(1),\
SRCDIR=$(S2OS_SRCDIR),\
DSTDIR=$(S2OS_DSTDIR),\
INS=$(S2OS_INS),\
OUT=$(S2OS_OUT),\
FLAGS=$(S2OS_FLAGS))"),)

$(if $(subst 1,,$(words $(S2OS_INS))),\
$(foreach in,$(S2OS_INS),\
$(call _S2O,$(1),$(S2OS_SRCDIR),$(S2OS_DSTDIR),$(in),$(in),$(S2OS_FLAGS))),\
$(call _S2O,$(1),$(S2OS_SRCDIR),$(S2OS_DSTDIR),$(S2OS_INS),$(S2OS_OUT),$(S2OS_FLAGS)))
endef


# *.o -> bin
define _LINK
$(eval LINK_SRCDIR = $(2))
$(eval LINK_DSTDIR = $(3))
$(eval LINK_INS = $(4))
$(eval LINK_OUT = $(5))
$(eval LINK_LIBS = $(6))
$(eval LINK_FLAGS = $(7))
$(eval LINK_C = $(8))
$(eval LINK_OBJS = $(addsuffix .o,$(LINK_INS)))

$(if $(DEBUG_RULES),$(warning "LINK($(1),\
SRCDIR=$(LINK_SRCDIR),\
DSTDIR=$(LINK_DSTDIR),\
INS=$(LINK_INS),\
OUT=$(LINK_OUT),\
LIBS=$(LINK_LIBS),\
C=$(LINK_C))"),)

$(eval LINK_DBG = $(findstring $(DEBUG),$(LINK_FLAGS)))
$(eval LINK_CFLAGS = $(if $(LINK_DBG),$(GCC_DBG_CFLAGS),$(GCC_CFLAGS)))

$(LINK_DSTDIR)$(LINK_OUT): $(addprefix $(LINK_SRCDIR),$(LINK_OBJS))
	@echo $$@: $$^
	$(if $(LINK_C),$($(1)_GCC) $(LINK_CFLAGS),$($(1)_LD)) -o $$@ $$^ $(LINK_LIBS) \
		$(if $(LINK_C),-lm,)

endef


# *.s -> bin
define _SNLINK
$(eval SNLINK_SRCDIR = $(2))
$(eval SNLINK_DSTDIR = $(3))
$(eval SNLINK_INS = $(4))
$(eval SNLINK_OUT = $(5))
$(eval SNLINK_LIBS = $(6))
$(eval SNLINK_FLAGS = $(7))
$(eval SNLINK_C = $(8))

$(if $(DEBUG_RULES),$(warning "SNLINK($(1),\
SRCDIR=$(SNLINK_SRCDIR),\
DSTDIR=$(SNLINK_DSTDIR),\
INS=$(SNLINK_INS),\
OUT=$(SNLINK_OUT),\
LIBS=$(SNLINK_LIBS),\
FLAGS=$(SNLINK_FLAGS),\
C=$(SNLINK_C))"),)

$(eval SNLINK_LDFLAGS = $(findstring $(DEBUG),$(SNLINK_FLAGS)))

$(call _S2OS,$(1),$(SNLINK_SRCDIR),$(SNLINK_DSTDIR),$(SNLINK_INS),\
$(SNLINK_OUT),$(SNLINK_FLAGS))

$(call _LINK,$(1),$(SNLINK_DSTDIR),$(SNLINK_DSTDIR),\
$(if $(subst 1,,$(words $(SNLINK_INS))),$(SNLINK_INS),$(SNLINK_OUT)),\
$(SNLINK_OUT),$(SNLINK_LIBS),$(SNLINK_LDFLAGS),$(SNLINK_C))
endef


# *.c -> bin
define _CNLINK
$(eval CNLINK_SRCDIR = $(2))
$(eval CNLINK_DSTDIR = $(3))
$(eval CNLINK_INS = $(4))
$(eval CNLINK_OUT = $(5))
$(eval CNLINK_LIBS = $(6))
$(eval CNLINK_FLAGS = $(7))
$(eval CNLINK_C = $(8))

$(if $(DEBUG_RULES),$(warning "CNLINK($(1),\
SRCDIR=$(CNLINK_SRCDIR),\
DSTDIR=$(CNLINK_DSTDIR),\
INS=$(CNLINK_INS),\
OUT=$(CNLINK_OUT),\
LIBS=$(CNLINK_LIBS),\
FLAGS=$(CNLINK_FLAGS),\
C=$(CNLINK_C))"),)

$(eval CNLINK_LDFLAGS = $(findstring $(DEBUG),$(CNLINK_FLAGS)))

$(call _C2S,$(1),$(CNLINK_SRCDIR),$(CNLINK_DSTDIR),$(CNLINK_INS),\
$(CNLINK_OUT),$(CNLINK_LIBS),$(CNLINK_FLAGS))

$(call _S2O,$(1),$(CNLINK_DSTDIR),$(CNLINK_DSTDIR),\
$(CNLINK_OUT),$(CNLINK_OUT))

$(call _LINK,$(1),$(CNLINK_DSTDIR),$(CNLINK_DSTDIR),\
$(CNLINK_OUT),$(CNLINK_OUT),$(CNLINK_LIBS),$(CNLINK_LDFLAGS),$(CNLINK_C))
endef


# clean
define _CLEAN
$(eval CLEAN_DIR = $(1))
$(eval CLEAN_MOD = $(2))
$(eval CLEAN_S = $(3))

$(if $(DEBUG_RULES),$(warning "CLEAN(DIR=$(CLEAN_DIR),\
MOD=$(CLEAN_MOD),\
CLEAN_S=$(CLEAN_S))"),)

$(CLEAN_MOD)-clean:
	@echo $$@:
	if [ -d $(CLEAN_DIR) ]; then \
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
"""


def main():
    _c2lbc(RV32, "srcdir", "dstdir", ["hello1", "hello2"], "hello", "", "-DROWS=3")

main()
