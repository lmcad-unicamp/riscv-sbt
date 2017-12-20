#!/usr/bin/env python3

from auto.config import *
from auto.utils import cat, cd, path, shell

import os


class Package:
    pkgs = []
    make_opts = None

    def __init__(self, name, prefix, build_dir,
            makefile="Makefile", configure=None, build_out=None, toolchain=None,
            deps=None):
        self.name = name
        self.prefix = prefix
        self.build_dir = build_dir
        self.makefile = makefile
        self._configure = configure
        self.build_out = build_out
        self.toolchain = toolchain
        self.deps = deps
        self.build_target = None


    def _clean(self):
        shell("rm -rf " + self.build_dir)


    def clean(self):
        # safety check
        if not self.build_dir or len(self.build_dir) < 3:
            raise Exception("dir to clean doesn't seem right, aborting...")
        self._clean()


    def do_deps(self):
        if self.deps:
            for dep in self.deps:
                for pkg in Package.pkgs:
                    if pkg.name == dep:
                        pkg.build_and_install()
                        break
                else:
                    raise Exception("Dependency not found: " + dep)


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
            Package.make_opts if target != "install" else ''))


    def _build(self):
        self._make(self.build_target)


    def build(self):
        if not os.path.exists(path(self.build_dir, self.build_out)):
            self.configure()
            print("*** building {} ***".format(self.name))
            self._build()


    def _install(self):
        self._make("install")


    def install(self):
        if not os.path.exists(path(self.prefix, self.toolchain)):
            self.build()

            print("*** installing {} ***".format(self.name))
            self._install()
            self.postinstall()


    def postinstall(self):
        pass


    def build_and_install(self):
        if not os.path.exists(DIR.build):
            shell("mkdir " + DIR.build)

        self.do_deps()
        self.build()
        self.install()

