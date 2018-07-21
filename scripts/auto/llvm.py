#!/usr/bin/env python3

from auto.config import DIR, RV32_LINUX
from auto.gnu_toolchain import GnuToolchain
from auto.utils import cat, cd, path, shell

import auto.pkg

import os


class LLVM(auto.pkg.Package):
    def _prepare(self):
        link = path(DIR.submodules, "llvm/tools/clang")
        if not os.path.exists(link):
            shell("ln -sf {}/clang {}".format(DIR.submodules, link))

        # Apply patches
        llvm_dir = path(DIR.submodules, "llvm")
        clang_dir = path(DIR.submodules, "clang")
        with cd(DIR.patches):
            llvm_patches = shell("ls 0?-relax-*", save_out=True).strip().split()
            clang_patches = shell("ls 0?-clang-*", save_out=True).strip().split()

        if not os.path.exists(path(llvm_dir, ".patched")):
            with cd(llvm_dir):
                for p in llvm_patches:
                    shell("patch -p0 < {}/{}".format(DIR.patches, p))
                shell("touch .patched")

        if not os.path.exists(path(clang_dir, ".patched")):
            with cd(clang_dir):
                for p in clang_patches:
                    shell("patch -p0 < {}/{}".format(DIR.patches, p))
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

    def configure(clang_ver, prefix, gnu_tc_prefix):
        return cat(
            "cmake",
            "-G Ninja",
            '-DLLVM_TARGETS_TO_BUILD="ARM;X86"',
            "-DCMAKE_BUILD_TYPE=Debug",
            "-DBUILD_SHARED_LIBS=True",
            "-DLLVM_USE_SPLIT_DWARF=True",
            "-DLLVM_OPTIMIZED_TABLEGEN=True",
            "-DLLVM_BUILD_TESTS=True",
            "-DCMAKE_C_COMPILER=/usr/bin/clang-{0}",
            "-DCMAKE_CXX_COMPILER=/usr/bin/clang++-{0}",
            "-DGCC_INSTALL_PREFIX={2}",
            "-DLLVM_DEFAULT_TARGET_TRIPLE=" + RV32_LINUX.triple,
            '-DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD="RISCV"',
            "-DCMAKE_INSTALL_PREFIX={1}",
            path(DIR.submodules, "llvm")).format(
                clang_ver, prefix, gnu_tc_prefix)

    makefile = "build.ninja"
    out = "bin/clang"
    toolchain = "bin/clang"
    deps = ["riscv-gnu-toolchain-newlib"]

    pkg0 = LLVM(name, prefix, build_dir,
            makefile=makefile,
            configure=configure("3.9", prefix, GnuToolchain.PREFIX),
            build_out=out,
            toolchain=toolchain,
            deps=deps)

    name = "llvm-gcc7"
    prefix = path(DIR.toolchain_debug, "gcc7")
    build_dir = path(DIR.build, "llvm-gcc7")
    pkg1 = LLVM(name, prefix, build_dir,
            makefile=makefile,
            configure=configure("6.0", prefix, GnuToolchain.PREFIX_GCC7),
            build_out=out,
            toolchain=toolchain,
            deps=deps)

    return [pkg0, pkg1]


auto.pkg.Package.pkgs.extend(_pkgs())
