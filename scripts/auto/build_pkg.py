#!/usr/bin/env python3

import auto.pkg
import auto.gnu_toolchain
import auto.llvm
import auto.spike
import auto.qemu
import auto.sbt

import argparse


if __name__ == "__main__":
    pkgs = auto.pkg.Package.pkgs

    parser = argparse.ArgumentParser(description="build packages")
    parser.add_argument("pkg")
    parser.add_argument("--clean", action="store_true")
    parser.add_argument("-j", type=int, default=9)
    args = parser.parse_args()

    auto.pkg.Package.make_opts = "-j" + str(args.j)

    if args.pkg == "list":
        for p in pkgs:
            print(p.name)
        exit()

    pkg = None
    for p in pkgs:
        if p.name == args.pkg:
            pkg = p
            break

    if not pkg:
        raise Exception("pkg not found!")

    if args.clean:
        pkg.clean()
    else:
        print("building", pkg.name)
        pkg.build_and_install()
