#!/usr/bin/env python3

from auto.config import *
import auto.pkg

import os


class LLVM(auto.pkg.Package):
    def _prepare(self):
        link = path(DIR.submodules, "llvm/tools/clang")
        if not os.path.exists(link):
            shell("ln -sf {}/clang {}".format(DIR.submodules, link))
        if not os.path.exists(path(link, ".patched")):
            with cd(link):
                shell("patch -p0 < baremetal.patch")
                shell("touch .patched")


    def _build(self):
        shell("cmake --build {} -- {}".format(self.build_dir, self.make_opts))


    def _install(self):
        shell("cmake --build {} --target install".format(self.build_dir))


    def _postinstall(self):
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
        "-DCMAKE_C_COMPILER=/usr/bin/clang-3.9",
        "-DCMAKE_CXX_COMPILER=/usr/bin/clang++-3.9",
        "-DDEFAULT_SYSROOT=" + RV32_LINUX.sysroot,
        "-DGCC_INSTALL_PREFIX=" + gnu_toolchain_prefix,
        "-DLLVM_DEFAULT_TARGET_TRIPLE=" + RV32_LINUX.triple,
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
