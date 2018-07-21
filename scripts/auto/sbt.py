#!/usr/bin/env python3

from auto.config import *
from auto.pkg import Package

import os


def _pkgs():
    # sbt debug
    name = "sbt"
    prefix = DIR.toolchain_debug
    build_dir = path(DIR.build, name)
    build_out = "riscv-sbt"
    toolchain = "bin/riscv-sbt"

    def configure(clang_ver, prefix):
        return cat(
            "cmake",
            "-DCMAKE_BUILD_TYPE=Debug",
            "-DCMAKE_C_COMPILER=/usr/bin/clang-{0}",
            "-DCMAKE_CXX_COMPILER=/usr/bin/clang++-{0}",
            "-DCMAKE_INSTALL_PREFIX={1}",
            path(DIR.top, "sbt")).format(clang_ver, prefix)

    deps = ["llvm"]

    pkg0 = Package(name, prefix, build_dir,
        configure=configure("3.9", prefix),
        build_out=build_out,
        toolchain=toolchain,
        deps=deps)

    name = "sbt-gcc7"
    prefix = path(DIR.toolchain_debug, "gcc7")
    build_dir = path(DIR.build, name)
    pkg1 = Package(name, prefix, build_dir,
        configure=configure("6.0", prefix),
        build_out=build_out,
        toolchain=toolchain,
        deps=deps)

    return [pkg0, pkg1]


Package.pkgs.extend(_pkgs())
