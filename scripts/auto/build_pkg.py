#!/usr/bin/env python3

from auto.utils import cat, cd, path, shell
import auto.gnu_toolchain

import argparse
import os

class Package:
    def __init__(self, name, prefix, build_dir,
            makefile="Makefile", configure=None, toolchain=None):
        self.name = name
        self.prefix = prefix
        self.build_dir = build_dir
        self.makefile = makefile
        self._configure = configure
        self.toolchain = toolchain
        self.make_opts = "-j9"


    def clean(self):
        # safety check
        if not self.build_dir or len(self.build_dir) < 3:
            raise Exception("dir to clean doesn't seem right, aborting...")
        shell("rm -rf " + self.build_dir)


    def prepare(self):
        pass


    def configure(self):
        # TODO check that self.deps exist

        if not self._configure:
            raise Exception("configure not set!")

        self.prepare()

        if not os.path.exists(self.build_dir):
            shell("mkdir -p " + self.build_dir)

        if not os.path.exists(path(self.build_dir, self.makefile)):
            with cd(self.build_dir):
                print("*** configuring " + self.name + " ***")
                shell(self._configure)


    def _make(self, target=None):
        shell(cat(
            "make -C",
            self.build_dir,
            target,
            self.make_opts if target != "install" else ''))


    def build(self):
        if not os.path.exists(path(self.build_dir, self.build_out)):
            self.configure()

            print("*** building {} ***".format(self.name))
            self._make(self.build_target)


    def install(self):
        if not os.path.exists(path(self.prefix, self.toolchain)):
            self.build()

            print("*** installing {} ***".format(self.name))
            self._make("install")


    def build_and_install(self):
        self.build()
        self.install()


if __name__ == "__main__":
    pkgs = auto.gnu_toolchain.pkgs()

    parser = argparse.ArgumentParser(description="build packages")
    parser.add_argument("pkg")
    parser.add_argument("--clean", action="store_true")
    args = parser.parse_args()

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
