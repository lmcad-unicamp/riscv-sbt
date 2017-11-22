### config ###

#
# dirs
#

TOOLCHAIN         := $(TOPDIR)/toolchain
TOOLCHAIN_RELEASE := $(TOOLCHAIN)/release
TOOLCHAIN_DEBUG   := $(TOOLCHAIN)/debug

# build type
BUILD_TYPE ?= Debug
ifeq ($(BUILD_TYPE),Debug)
  BUILD_TYPE_DIR  := debug
else
  BUILD_TYPE_DIR  := release
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

MAKE_OPTS      ?= -j9
CFLAGS         := -fno-rtti -fno-exceptions

#
# SBT
#

SBTFLAGS           = -x
SBT_SHARE_DIR     := $(TOOLCHAIN_DIR)/share/riscv-sbt

SBT_NAT_OBJS      := SYSCALL
X86_SYSCALL_O     := $(SBT_SHARE_DIR)/x86-syscall.o
X86_COUNTERS_O    := $(SBT_SHARE_DIR)/x86-counters.o

#
# tools
#

CMAKE             := cmake

LOG               := $(SCRIPTS_DIR)/run.sh --log
RUN_SH            := $(SCRIPTS_DIR)/run.sh

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
RV32_MARCH        := riscv32
RV32_LINUX_MARCH  := $(RV32_MARCH)

# RV32_LINUX_ABI    := ilp32d
RV32_LINUX_ABI    := ilp32

#
# gcc
#

RV32_GCC       := $(RV32_TRIPLE)-gcc
RV32_LINUX_GCC := $(RV64_LINUX_TRIPLE)-gcc -march=rv32g -mabi=$(RV32_LINUX_ABI)
X86_GCC        := gcc -m32

_O             := -O3

GCC_CFLAGS     := -static $(_O) $(CFLAGS)
# debug
# GCC_CFLAGS     := -static -O0 -g

#
# clang
#

CLANG             := clang
RV32_CLANG        := $(CLANG) --target=riscv32
RV32_LINUX_CLANG  := $(CLANG) --target=riscv32
X86_CLANG         := $(CLANG) --target=x86_64-unknown-linux-gnu -m32

RV32_SYSROOT            := $(TOOLCHAIN_RELEASE)/opt/riscv/$(RV32_TRIPLE)
RV32_SYSROOT_FLAG       := -isysroot $(RV32_SYSROOT) -isystem $(RV32_SYSROOT)/include
RV32_CLANG_FLAGS         = $(RV32_SYSROOT_FLAG)

RV64_LINUX_SYSROOT      := $(TOOLCHAIN_RELEASE)/opt/riscv/sysroot
RV64_LINUX_SYSROOT_FLAG := -isysroot $(RV64_LINUX_SYSROOT) -isystem $(RV64_LINUX_SYSROOT)/usr/include
RV32_LINUX_CLANG_FLAGS   = $(RV64_LINUX_SYSROOT_FLAG) -D__riscv_xlen=32

EMITLLVM          := -emit-llvm -c $(_O) -mllvm -disable-llvm-optzns

LLC               := llc
LLC_FLAGS         := -relocation-model=static $(_O) #-stats
RV32_LLC_FLAGS    := -march=$(RV32_MARCH) -mattr=+m
RV32_LINUX_LLC_FLAGS := $(RV32_LLC_FLAGS)
X86_LLC_FLAGS     := -march=$(X86_MARCH) -mattr=avx #-mattr=avx2

LLVMOPT           := opt
LLVMOPT_FLAGS     := $(_O) #-stats

LLVMDIS           := llvm-dis
LLVMLINK          := llvm-link

#
# AS
#

RV32_AS           := $(RV32_TRIPLE)-as
RV64_LINUX_AS     := $(RV64_LINUX_TRIPLE)-as
RV32_LINUX_AS     := $(RV64_LINUX_AS) -march=rv32g -mabi=$(RV32_LINUX_ABI)
X86_AS            := $(X86_64_TRIPLE)-as --32 #-g

#
# LD
#

RV32_LD           := $(RV32_TRIPLE)-ld
RV64_LINUX_LD     := $(RV64_LINUX_TRIPLE)-ld
RV32_LINUX_LD     := $(RV64_LINUX_LD) -m elf32lriscv
X86_LD            := $(X86_64_TRIPLE)-ld -m elf_i386
