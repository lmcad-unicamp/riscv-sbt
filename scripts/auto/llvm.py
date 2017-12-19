#!/usr/bin/env python3

from auto.config import *
import auto.pkg

import os


class LLVM(auto.pkg.Package):
    def prepare(self):
        link = path(DIR.submodules, "llvm/tools/clang")
        if not os.path.exists(link):
            shell("ln -sf {}/clang {}".format(DIR.submodules, link))


    def _build(self):
        shell("cmake --build {} -- {}".format(self.build_dir, self.make_opts))


    def _install(self):
        shell("cmake --build {} --target install".format(self.build_dir))


    def postinstall(self):
        srcdir = path(self.build_dir, "lib/Target/RISCV")
        dstdir = path(self.prefix, "include/llvm/Target/RISCV")

        shell("mkdir -p " + dstdir)
        for f in ["RISCVGenInstrInfo.inc", "RISCVGenRegisterInfo.inc"]:
            shell("cp {0}/{2} {1}/{2}".format(srcdir, dstdir, f))


def _pkgs():
    # lowrisc llvm
    name = "llvm"
    prefix = DIR.toolchain_debug
    build_dir = path(DIR.build, "llvm")

    gnu_toolchain_prefix = auto.gnu_toolchain.GNU_TOOLCHAIN_PREFIX

    configure = cat(
        "cmake",
        "-G Ninja",
        '-DLLVM_TARGETS_TO_BUILD="ARM;X86"',
        "-DCMAKE_BUILD_TYPE=Debug",
        "-DBUILD_SHARED_LIBS=True",
        "-DLLVM_USE_SPLIT_DWARF=True",
        "-DLLVM_OPTIMIZED_TABLEGEN=True",
        "-DLLVM_BUILD_TESTS=True",
        "-DDEFAULT_SYSROOT=" + path(gnu_toolchain_prefix, RV32.triple),
        "-DGCC_INSTALL_PREFIX=" + gnu_toolchain_prefix,
        "-DLLVM_DEFAULT_TARGET_TRIPLE=" + RV32.triple,
        '-DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD="RISCV"',
        "-DCMAKE_INSTALL_PREFIX=" + prefix,
        path(DIR.submodules, "llvm")
        )

    makefile = "build.ninja"
    out = "bin/clang"
    toolchain = "bin/clang"
    deps = ["riscv-gnu-toolchain-newlib"]

    pkg = LLVM(name, prefix, build_dir,
            makefile=makefile,
            configure=configure,
            build_out=out,
            toolchain=toolchain,
            deps=deps)

    return [pkg]


auto.pkg.Package.pkgs.extend(_pkgs())
