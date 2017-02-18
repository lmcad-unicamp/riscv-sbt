### config ###

# flags
CFLAGS    = -Wall -Werror -O2
CXXFLAGS  = $(CFLAGS) -std=c++11
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
else
  BUILD_TYPE_DIR := release
endif

TOOLCHAIN_DIR  := $(TOOLCHAIN)/$(BUILD_TYPE_DIR)

#
# tools
#

SBT_SHARE_DIR := $(TOOLCHAIN_DIR)/share/riscv-sbt
X86_SYSCALL_O := $(SBT_SHARE_DIR)/x86-syscall.o
X86_RVSC_O    := $(SBT_SHARE_DIR)/x86-rvsc.o

RV32_TRIPLE   := riscv32-unknown-elf
RV64_TRIPLE   := riscv64-unknown-elf
X86_TRIPLE    := i386-unknown-elf

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
RV32_CLANG_FLAGS  := --target=riscv32 -mriscv=RV32IAMFD \
                     $(RV32_SYSROOT_FLAG)
RV32_LIB      := $(TOOLCHAIN_RELEASE)/$(RV32_TRIPLE)/lib
RV32_LIB_GCC  := $(TOOLCHAIN_RELEASE)/lib/gcc/$(RV32_TRIPLE)/7.0.0
RV32_CRT0     := $(RV32_LIB)/crt0.o
RV32_LD_FLAGS0 := -L$(RV32_LIB) -L$(RV32_LIB_GCC) \
                  -dT ldscripts/elf32lriscv.x $(RV32_CRT0)
RV32_LD_FLAGS1 := -lc -lgloss -lc -lgcc

# x86
X86_CLANG       := $(CLANG)
X86_CLANG_FLAGS := --target=i386 -fno-exceptions -fno-rtti
X86_LIB         := /usr/lib32
X86_LIB_GCC     := $(TOOLCHAIN_X86)/lib/gcc/$(X86_TRIPLE)/7.0.0
X86_CRT1        := $(X86_LIB)/crt1.o
X86_CRTI        := $(X86_LIB)/crti.o
X86_CRTBEGIN    := $(X86_LIB_GCC)/crtbegin.o
X86_CRTEND      := $(X86_LIB_GCC)/crtend.o
X86_CRTN        := $(X86_LIB)/crtn.o
X86_LGCC        := $(X86_LIB_GCC)/libgcc.a
X86_LC          := $(X86_LIB)/libc.a
X86_LD_FLAGS0   := $(X86_CRT1) $(X86_CRTI)
X86_LD_FLAGS1   := $(X86_LC) $(X86_LGCC) $(X86_LC) $(X86_CRTN)
