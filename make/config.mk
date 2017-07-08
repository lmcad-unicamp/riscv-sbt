### config ###

# flags
CFLAGS    = -Wall -Werror -O3
CXXFLAGS  = $(CFLAGS) -std=c++11
LLCFLAGS  = -O3
SBTFLAGS  = -x
MAKE_OPTS ?= -j9

# toolchain
TOOLCHAIN         := $(TOPDIR)/toolchain
TOOLCHAIN_RELEASE := $(TOOLCHAIN)/release
TOOLCHAIN_DEBUG   := $(TOOLCHAIN)/debug
TOOLCHAIN_X86     := $(TOOLCHAIN)/x86

# build type
BUILD_TYPE ?= Debug
ifeq ($(BUILD_TYPE),Debug)
  BUILD_TYPE_DIR := debug
  LLVM_INSTALL_DIR := $(TOOLCHAIN_DEBUG)/llvm
else
  BUILD_TYPE_DIR := release
  LLVM_INSTALL_DIR := $(TOOLCHAIN_RELEASE)/llvm
endif


TOOLCHAIN_DIR  := $(TOOLCHAIN)/$(BUILD_TYPE_DIR)
REMOTE_DIR     := $(TOPDIR)/remote
BUILD_DIR      := $(TOPDIR)/build
PATCHES_DIR    := $(TOPDIR)/patches
SUBMODULES_DIR := $(TOPDIR)/submodules

MAKE_DIR =

#
# tools
#

SBT_SHARE_DIR := $(TOOLCHAIN_DIR)/share/riscv-sbt
X86_SYSCALL_O := $(SBT_SHARE_DIR)/x86-syscall.o
X86_RVSC_O    := $(SBT_SHARE_DIR)/x86-rvsc.o
X86_DUMMY_O   := $(TOPDIR)/sbt/test/x86-dummy.o

RV32_TRIPLE       := riscv32-unknown-elf
RV32_LINUX_TRIPLE := riscv32-unknown-linux-gnu
RV64_TRIPLE       := riscv64-unknown-elf
RV64_LINUX_TRIPLE := riscv64-unknown-linux-gnu
X86_TRIPLE        := i386-unknown-elf

RV32_AS   := $(RV32_TRIPLE)-as
RV32_LD   := $(RV32_TRIPLE)-ld
RV32_GCC  := $(RV32_TRIPLE)-gcc

RV64_AS   := $(RV64_TRIPLE)-as
RV64_LD   := $(RV64_TRIPLE)-ld
RV64_GCC  := $(RV64_TRIPLE)-gcc

RV32_RUN  := LD_LIBRARY_PATH=$(TOOLCHAIN_RELEASE)/lib spike $(PK32)
RV64_RUN  := LD_LIBRARY_PATH=$(TOOLCHAIN_RELEASE)/lib spike --isa=RV64IMAFDC pk

X86_AS    := $(X86_TRIPLE)-as
X86_LD    := $(X86_TRIPLE)-ld

# clang
CLANG := clang


# RISC-V 32 bit
RV32_CLANG        := $(CLANG)
RV32_SYSROOT      := $(TOOLCHAIN_RELEASE)/$(RV32_TRIPLE)
RV32_SYSROOT_FLAG := -isysroot $(RV32_SYSROOT) \
                     -isystem $(RV32_SYSROOT)/include
RV32_CLANG_FLAGS  := --target=riscv -mriscv=RV32IAMFD \
                     $(RV32_SYSROOT_FLAG)
RV32_LIB      := $(TOOLCHAIN_RELEASE)/$(RV32_TRIPLE)/lib
RV32_LIB_GCC  := $(TOOLCHAIN_RELEASE)/lib/gcc/$(RV32_TRIPLE)/7.1.1
RV32_CRT0     := $(RV32_LIB)/crt0.o
RV32_LD_FLAGS0 := -L$(RV32_LIB) -L$(RV32_LIB_GCC) \
                  -dT ldscripts/elf32lriscv.x $(RV32_CRT0)
RV32_LD_FLAGS1 := -lc -lgloss -lc -lgcc
RV32_PREFIX    := rv32


# RISC-V 64 bit
RV64_CLANG        := $(CLANG)
RV64_SYSROOT      := $(TOOLCHAIN_RELEASE)/$(RV64_TRIPLE)
RV64_SYSROOT_FLAG := -isysroot $(RV64_SYSROOT) \
                     -isystem $(RV64_SYSROOT)/include
RV64_CLANG_FLAGS  := --target=riscv64 -mriscv=RV64IAMFD \
                     $(RV64_SYSROOT_FLAG)
RV64_LIB      := $(TOOLCHAIN_RELEASE)/$(RV64_TRIPLE)/lib
RV64_LIB_GCC  := $(TOOLCHAIN_RELEASE)/lib/gcc/$(RV64_TRIPLE)/7.1.1
RV64_CRT0     := $(RV64_LIB)/crt0.o
RV64_LD_FLAGS0 := -L$(RV64_LIB) -L$(RV64_LIB_GCC) \
                  -dT ldscripts/elf64lriscv.x $(RV64_CRT0)
RV64_LD_FLAGS1 := -lc -lgloss -lc -lgcc
RV64_PREFIX    := rv64


# x86
X86_CLANG       := $(CLANG)
X86_CLANG_FLAGS := --target=i386 -fno-exceptions -fno-rtti
X86_LIB         := /usr/lib32
X86_LIB_GCC     := $(TOOLCHAIN_X86)/lib/gcc/$(X86_TRIPLE)/7.1.1
X86_CRT1        := $(X86_LIB)/crt1.o
X86_CRTI        := $(X86_LIB)/crti.o
X86_CRTBEGIN    := $(X86_LIB_GCC)/crtbegin.o
X86_CRTEND      := $(X86_LIB_GCC)/crtend.o
X86_CRTN        := $(X86_LIB)/crtn.o
X86_LGCC        := $(X86_LIB_GCC)/libgcc.a
X86_LC          := $(X86_LIB)/libc.a
X86_LD_FLAGS0   := $(X86_CRT1) $(X86_CRTI)
X86_LD_FLAGS1   := $(X86_LC) $(X86_LGCC) $(X86_LC) $(X86_CRTN)
X86_MARCH       := x86
X86_PREFIX      := x86
