### config ###

#
# dirs
#

TOOLCHAIN         := $(TOPDIR)/toolchain
TOOLCHAIN_RELEASE := $(TOOLCHAIN)/release
TOOLCHAIN_DEBUG   := $(TOOLCHAIN)/debug
TOOLCHAIN_X86     := $(TOOLCHAIN)/x86

# build type
BUILD_TYPE ?= Debug
ifeq ($(BUILD_TYPE),Debug)
  BUILD_TYPE_DIR  := debug
  LLVM_INSTALL_DIR := $(TOOLCHAIN_DEBUG)/llvm
else
  BUILD_TYPE_DIR  := release
  LLVM_INSTALL_DIR := $(TOOLCHAIN_RELEASE)/llvm
endif

TOOLCHAIN_DIR  := $(TOOLCHAIN)/$(BUILD_TYPE_DIR)

REMOTE_DIR     := $(TOPDIR)/remote
BUILD_DIR      := $(TOPDIR)/build
PATCHES_DIR    := $(TOPDIR)/patches
SCRIPTS_DIR    := $(TOPDIR)/scripts
SUBMODULES_DIR := $(TOPDIR)/submodules

#
# flags
#

MAKE_OPTS     ?= -j9

#
# SBT
#

SBTFLAGS        = #-x
SBT_SHARE_DIR  := $(TOOLCHAIN_DIR)/share/riscv-sbt
X86_SYSCALL_O  := $(SBT_SHARE_DIR)/x86-syscall.o
X86_COUNTERS_O := $(SBT_SHARE_DIR)/x86-counters.o

#
# tools
#

LOG               := $(SCRIPTS_DIR)/run.sh

RV32_TRIPLE       := riscv32-unknown-elf
RV64_LINUX_TRIPLE := riscv64-unknown-linux-gnu
X86_64_TRIPLE     := x86_64-linux-gnu

RV32_RUN          := LD_LIBRARY_PATH=$(TOOLCHAIN_RELEASE)/lib spike $(PK32)
RV32_LINUX_RUN    := qemu-riscv32
X86_RUN           :=

RV32_PREFIX       := rv32
RV32_LINUX_PREFIX := $(RV32_PREFIX)
X86_PREFIX        := x86

X86_MARCH         := x86
RV32_MARCH        := riscv
RV32_LINUX_MARCH  := $(RV32_MARCH)

#
# gcc
#

RV32_GCC       := $(RV32_TRIPLE)-gcc
RV32_LINUX_GCC := $(RV64_LINUX_TRIPLE)-gcc -march=rv32g -mabi=ilp32d
X86_GCC        := gcc -m32

#
# clang
#

CLANG             := clang
RV32_CLANG        := $(CLANG) --target=riscv -mriscv=RV32IMAFD
RV32_LINUX_CLANG  := $(CLANG) --target=riscv -mriscv=RV32IMAFD
X86_CLANG         := $(CLANG) --target=x86_64-unknown-linux-gnu -m32

CLANG_FLAGS       := -fno-rtti -fno-exceptions

RV32_SYSROOT      := $(TOOLCHAIN_RELEASE)/$(RV32_TRIPLE)
RV32_SYSROOT_FLAG := -isysroot $(RV32_SYSROOT) -isystem $(RV32_SYSROOT)/include
RV32_CLANG_FLAGS   = $(RV32_SYSROOT_FLAG)

RV64_LINUX_SYSROOT      := $(TOOLCHAIN_RELEASE)/sysroot
RV64_LINUX_SYSROOT_FLAG := -isysroot $(RV64_LINUX_SYSROOT) -isystem $(RV64_LINUX_SYSROOT)/usr/include
RV32_LINUX_CLANG_FLAGS   = $(RV64_LINUX_SYSROOT_FLAG) -D__riscv_xlen=32

EMITLLVM          := -emit-llvm -c -O3 -mllvm -disable-llvm-optzns

LLC               := llc
LLC_FLAGS         := -relocation-model=static -O3 #-stats
RV32_LLC_FLAGS    := -march=$(RV32_MARCH) -mcpu=RV32IMAFD
RV32_LINUX_LLC_FLAGS := $(RV32_LLC_FLAGS)
X86_LLC_FLAGS     := -march=$(X86_MARCH) -mattr=avx #-mattr=avx2

LLVMOPT           := $(LLVM_INSTALL_DIR)/bin/opt
LLVMOPT_FLAGS     := -O3 #-stats

LLVMDIS           := $(LLVM_INSTALL_DIR)/bin/llvm-dis
LLVMLINK          := $(LLVM_INSTALL_DIR)/bin/llvm-link

#
# AS
#

RV32_AS           := $(RV32_TRIPLE)-as
RV64_LINUX_AS     := $(RV64_LINUX_TRIPLE)-as
RV32_LINUX_AS     := $(RV64_LINUX_AS) -march=rv32g -mabi=ilp32d
X86_AS            := $(X86_64_TRIPLE)-as --32 #-g

#
# LD
#

RV32_LD           := $(RV32_TRIPLE)-ld
RV64_LINUX_LD     := $(RV64_LINUX_TRIPLE)-ld
RV32_LINUX_LD     := $(RV64_LINUX_LD) -m elf32lriscv
X86_LD            := $(X86_64_TRIPLE)-ld -m elf_i386

# RISC-V 32 bit

RV32_LIB          := $(TOOLCHAIN_RELEASE)/$(RV32_TRIPLE)/lib
RV32_LIB_GCC      := $(TOOLCHAIN_RELEASE)/lib/gcc/$(RV32_TRIPLE)/7.1.1
RV32_CRT0         := $(RV32_LIB)/crt0.o
RV32_LD_FLAGS0    := -L$(RV32_LIB) -L$(RV32_LIB_GCC) \
                     -dT ldscripts/elf32lriscv.x $(RV32_CRT0)
RV32_LD_FLAGS1    := -lm -lc -lgloss -lc -lgcc

RV32_LINUX_LIB       := $(RV64_LINUX_SYSROOT)/lib32/ilp32d
RV32_LINUX_USR_LIB   := $(RV64_LINUX_SYSROOT)/usr/lib32/ilp32d
RV32_LINUX_LIB_GCC   := $(TOOLCHAIN_RELEASE)/lib/gcc/$(RV64_LINUX_TRIPLE)/7.1.1/lib32/ilp32d
RV32_LINUX_CRT1      := $(RV32_LINUX_USR_LIB)/crt1.o
RV32_LINUX_CRTI      := $(RV32_LINUX_LIB_GCC)/crti.o
RV32_LINUX_CRTN      := $(RV32_LINUX_LIB_GCC)/crtn.o
RV32_LINUX_CRTBEGIN  := $(RV32_LINUX_LIB_GCC)/crtbeginT.o
RV32_LINUX_CRTEND    := $(RV32_LINUX_LIB_GCC)/crtend.o
RV32_LINUX_LD_FLAGS0 := -static -L$(RV32_LINUX_LIB_GCC) -L$(RV32_LINUX_USR_LIB) \
                        $(RV32_LINUX_CRT1) $(RV32_LINUX_CRTI) $(RV32_LINUX_CRTBEGIN)
RV32_LINUX_LD_FLAGS1 := -lm -lc -lgcc -lgcc_eh -lc \
                        $(RV32_LINUX_CRTEND) $(RV32_LINUX_CRTN)


# x86

X86_LIB         := /usr/lib32
X86_LIB_GCC     := /usr/lib/gcc/$(X86_64_TRIPLE)/4.9.2/32
X86_CRT1        := $(X86_LIB)/crt1.o
X86_CRTI        := $(X86_LIB)/crti.o
X86_CRTN        := $(X86_LIB)/crtn.o
X86_CRTBEGIN    := $(X86_LIB_GCC)/crtbegin.o
X86_CRTEND      := $(X86_LIB_GCC)/crtend.o
X86_LGCC        := $(X86_LIB_GCC)/libgcc.a
X86_LEH         := $(X86_LIB_GCC)/libgcc_eh.a
X86_LD_FLAGS0   := -static -L$(X86_LIB_GCC) \
                   $(X86_CRT1) $(X86_CRTI) $(X86_CRTBEGIN)
X86_LD_FLAGS1   := -lm -lc -lgcc -lgcc_eh -lc \
                   $(X86_CRTEND) $(X86_CRTN)


#
# extra (unused) stuff
#

RV32_LINUX_TRIPLE := riscv32-unknown-linux-gnu
RV64_TRIPLE       := riscv64-unknown-elf
X86_TRIPLE        := i386-unknown-elf

RV64_RUN          := LD_LIBRARY_PATH=$(TOOLCHAIN_RELEASE)/lib spike --isa=RV64IMAFDC pk
RV64_CLANG        := $(CLANG)

RV64_SYSROOT      := $(TOOLCHAIN_RELEASE)/$(RV64_TRIPLE)
RV64_SYSROOT_FLAG := -isysroot $(RV64_SYSROOT) \
                     -isystem $(RV64_SYSROOT)/include

RV64_CLANG_FLAGS  := --target=riscv64 -mriscv=RV64IMAFD $(RV64_SYSROOT_FLAG)

RV64_AS           := $(RV64_TRIPLE)-as
RV64_LD           := $(RV64_TRIPLE)-ld

# RISC-V 64 bit

RV64_LIB          := $(TOOLCHAIN_RELEASE)/$(RV64_TRIPLE)/lib
RV64_LIB_GCC      := $(TOOLCHAIN_RELEASE)/lib/gcc/$(RV64_TRIPLE)/7.1.1
RV64_CRT0         := $(RV64_LIB)/crt0.o
RV64_LD_FLAGS0    := -L$(RV64_LIB) -L$(RV64_LIB_GCC) \
                     -dT ldscripts/elf64lriscv.x $(RV64_CRT0)
RV64_LD_FLAGS1    := -lc -lgloss -lc -lgcc
RV64_PREFIX       := rv64
