#!/usr/bin/env python3

from auto.config import *
from auto.pkg import Package

import os


def _pkgs():
    pkgs = []

    # sbt debug
    name = "sbt"
    prefix = DIR.toolchain_debug
    build_dir = path(DIR.build, name)
    build_out = "riscv-sbt"
    toolchain = "bin/riscv-sbt"
    configure = cat(
        "cmake",
        "-DCMAKE_BUILD_TYPE=Debug",
        "-DCMAKE_C_COMPILER=/usr/bin/clang-3.9",
        "-DCMAKE_CXX_COMPILER=/usr/bin/clang++-3.9",
        "-DCMAKE_INSTALL_PREFIX=" + prefix,
        path(DIR.top, "sbt")
    )
    deps = ["llvm"]

    pkgs.append(
        Package(name, prefix, build_dir,
            configure=configure,
            build_out=build_out,
            toolchain=toolchain,
            deps=deps))

    return pkgs


Package.pkgs.extend(_pkgs())
