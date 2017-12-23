#!/usr/bin/env python3

from auto.utils import cat, cd, mkdir_if_needed, mpath, path, shell

import argparse
import re
import os
import sys

TOPDIR          = os.environ["TOPDIR"]
DOCKER_DIR      = path(TOPDIR, "docker")
OUTPUT_DIR      = path(DOCKER_DIR, "output")
SRC_DIR         = path(OUTPUT_DIR, "src")

class Image:
    def __init__(self, name, srcs=None, img=None):
        self.name = name
        if srcs == None:
            self.srcs = [name]
        else:
            self.srcs = srcs
        if not img:
            self.img = "sbt-" + name
        else:
            self.img = img


    def copy_src(self):
        if len(self.srcs) == 0:
            return

        dstdir = mpath(DOCKER_DIR, self.name, "src")

        if len(self.srcs) == 1:
            if not os.path.exists(dstdir):
                # NOTE use -rL if you get issues on Windows
                shell("cp -a {} {}".format(
                    path(SRC_DIR, self.srcs[0]),
                    dstdir))
        else:
            mkdir_if_needed(dstdir)

            for src in self.srcs:
                dir = path(dstdir, src)
                if not os.path.exists(dir):
                    shell("cp -a {} {}".format(
                        path(SRC_DIR, src),
                        dir))


    def build(self):
        dir = path(DOCKER_DIR, self.name)
        done = path(dir, "done")
        if os.path.exists(done):
            return

        self.copy_src()
        with cd(DOCKER_DIR):
            shell("docker build -t {} {}".format(
                self.img, self.name))
            shell("touch " + done)


class Commit:
    def __init__(self, module, commit):
        self.module = module
        self.commit = commit


class Commits:
    def __init__(self, topdir, docker_dir):
        self.topdir = topdir
        self.commits_txt = path(docker_dir, "commits.txt")


    def get_current(self):
        commits = []

        with cd(self.topdir):
            # get submodules
            out = shell("git submodule", save_out=True)
            lines = out.split("\n")
            patt = re.compile(" *([^ ]+) +submodules/([^ ]+) *")
            for l in lines:
                r = re.match(patt, l)
                if r:
                    commit = r.group(1)
                    module = r.group(2)
                    commits.append(Commit(module, commit))

            # add sbt
            commits.append(Commit("riscv-sbt",
                shell("git rev-parse HEAD", save_out=True)
                    .format(self.topdir)
                    .strip()))
            return commits


    def save(self, commits):
        with open(self.commits_txt, "w") as f:
            for commit in commits:
                f.write("{} {}\n".format(commit.module, commit.commit))


    def load(self):
        self.commits = {}
        with open(self.commits_txt, "r") as f:
            patt = re.compile("([^ ]+) ([^ ]+)")
            for line in f:
                r = re.match(patt, line)
                self.commits[r.group(1)] = r.group(2)


    def __getitem__(self, key):
        return self.commits[key]


def get_srcs():
    commits = Commits(TOPDIR, DOCKER_DIR)
    commits.load()

    class Source:
        srcdir = SRC_DIR

        def __init__(self, name, url, clone_flags=None):
            self.name = name
            self.url = url
            self.clone_flags = clone_flags


        def get(self):
            dir = path(self.srcdir, self.name)

            if not os.path.exists(dir):
                with cd(self.srcdir):
                    cmd = cat("git clone", self.clone_flags, self.url)
                    shell(cmd)
                    with cd(self.name):
                        shell("git checkout " + self.commit)


    srcs = [
        Source("riscv-sbt",
            "https://github.com/OpenISA/riscv-sbt.git"),
        Source("riscv-gnu-toolchain",
            "https://github.com/riscv/riscv-gnu-toolchain",
            "--recursive"),
        Source("llvm",
            "https://github.com/OpenISA/llvm",
            "--recursive -b lowrisc"),
        Source("clang",
            "https://github.com/OpenISA/clang",
            "--recursive -b lowrisc"),
        Source("riscv-fesvr",
            "https://github.com/riscv/riscv-fesvr",
            "--recursive"),
        Source("riscv-pk",
            "https://github.com/riscv/riscv-pk",
            "--recursive"),
        Source("riscv-isa-sim",
            "https://github.com/riscv/riscv-isa-sim",
            "--recursive"),
        Source("riscv-qemu-tests",
            "https://github.com/arsv/riscv-qemu-tests",
            "--recursive")
    ]

    # set commits
    for src in srcs:
        src.commit = commits[src.name]

    mkdir_if_needed(SRC_DIR)

    # get all sources
    for src in srcs:
        src.get()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="docker build helper")
    parser.add_argument("--run", action="store_true")
    parser.add_argument("--clean-srcs", action="store_true")
    parser.add_argument("--clean-imgs", action="store_true")
    parser.add_argument("--clean", action="store_true")
    parser.add_argument("--save-current-commits", action="store_true")
    parser.add_argument("--get-srcs", action="store_true")
    parser.add_argument("--build")
    args = parser.parse_args()

    imgs = [
        Image("riscv-sbt"),
        Image("riscv-gnu-toolchain"),
        Image("emu",
            srcs=["riscv-fesvr", "riscv-isa-sim",
                "riscv-pk", "riscv-qemu-tests"]),
        Image("llvm", srcs=["llvm", "clang"]),
        Image("sbt", srcs=[], img="sbt")
    ]

    names = [img.name for img in imgs]

    # --run
    if args.run:
        shell("docker run -it --rm -h sbt sbt")
    # --clean-srcs
    elif args.clean_srcs:
        shell("rm -rf " + SRC_DIR)
    # --clean-imgs
    elif args.clean_imgs:
        # remove all container instances
        containers = shell("docker ps -a | awk '{print$1}' | sed 1d",
            save_out=True).split()
        if len(containers) > 0:
            shell("docker rm " + " ".join(containers))
        # remove all '<none>' and 'sbt' images
        images = shell(
            "docker images | grep -e sbt -e '<none>' | awk '{print$3}'",
            save_out=True).split()
        if len(images) > 0:
            shell("docker rmi " + " ".join(images))
    # --clean
    elif args.clean:
        with cd(DOCKER_DIR):
            shell("rm -rf {}".format(
                " ".join([name + "/src" for name in names])))
            shell("rm -f {}".format(
                " ".join([name + "/done" for name in names])))
    # --save-current-commits
    elif args.save_current_commits:
        commits = Commits(TOPDIR, DOCKER_DIR)
        commits.save(commits.get_current())
    # --get-srcs
    elif args.get_srcs:
        get_srcs()
    # --build
    elif args.build:
        for img in imgs:
            if img.name == args.build or args.build == "all":
                img.build()
                if args.build != "all":
                    break
        else:
            if args.build != "all":
                sys.exit("ERROR: component not found: " + args.build)
    # error
    else:
        sys.exit("ERROR: no command specified")
