### config ###

# dirs

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

# flags

MAKE_OPTS      ?= -j9
MODES          := globals locals

# tools

CMAKE             := cmake
MEASURE_PY        := $(SCRIPTS_DIR)/measure.py
BUILD_PY          := $(SCRIPTS_DIR)/build.py

RV32_TRIPLE       := riscv32-unknown-elf
RV64_LINUX_TRIPLE := riscv64-unknown-linux-gnu
X86_64_TRIPLE     := x86_64-linux-gnu

