#!/usr/bin/env python3

from auto.utils import cat, cd, mkdir_if_needed, mpath, path, shell

import argparse
import re
import os
import sys

TOPDIR          = os.environ["TOPDIR"]
DOCKER_DIR      = path(TOPDIR, "docker")

#
# Commits
#

# module + commit_hash
class Commit:
    def __init__(self, module, commit):
        self.module = module
        self.commit = commit


class Commits:
    def __init__(self, topdir, docker_dir):
        self.topdir = topdir
        self.commits_txt = path(docker_dir, "commits.txt")


    # get current commit hashes
    def get_current(self):
        commits = []

        with cd(self.topdir):
            # get submodules
            out = shell("git submodule", save_out=True)
            lines = out.split("\n")
            patt = re.compile("[ -]*([^ ]+) +submodules/([^ ]+) *")
            for l in lines:
                if l.find("lowrisc-llvm") >= 0:
                    continue
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


    # save to file
    def save(self, commits):
        with open(self.commits_txt, "w") as f:
            for commit in commits:
                f.write("{} {}\n".format(commit.module, commit.commit))


    # load from file
    def load(self):
        self.commits = {}
        with open(self.commits_txt, "r") as f:
            patt = re.compile("([^ ]+) ([^ ]+)")
            for line in f:
                r = re.match(patt, line)
                self.commits[r.group(1)] = r.group(2)
        return self


    def __getitem__(self, key):
        return self.commits[key]

#
# Sources
#

# source package
class Source:
    def __init__(self, name, dir, url, clone_flags=None):
        self.name = name
        self.parent_dir = mpath(DOCKER_DIR, dir, "src")
        self.dstdir = path(self.parent_dir, name)
        self.url = url
        self.clone_flags = clone_flags
        self.update = True


    def get(self):
        if not os.path.exists(self.dstdir):
            mkdir_if_needed(self.parent_dir)
            with cd(self.parent_dir):
                cmd = cat("git clone", self.clone_flags, self.url)
                shell(cmd)
        elif self.update:
            with cd(self.dstdir):
                shell("git fetch")

        with cd(self.dstdir):
            shell((
                "if [ `git rev-parse HEAD` != {0} ]; then " +
                    "git checkout {0}; fi").format(self.commit.strip()))


class Sources:
    def __init__(self):
        self.srcs = [
            Source("riscv-sbt", "riscv-sbt",
                "https://github.com/OpenISA/riscv-sbt.git"),
            Source("riscv-gnu-toolchain", "riscv-gnu-toolchain",
                "https://github.com/riscv/riscv-gnu-toolchain",
                "--recursive"),
            Source("llvm", "llvm",
                "https://github.com/OpenISA/llvm",
                "--recursive -b lowrisc"),
            Source("clang", "llvm",
                "https://github.com/OpenISA/clang",
                "--recursive -b lowrisc"),
            Source("riscv-fesvr", "emu",
                "https://github.com/riscv/riscv-fesvr",
                "--recursive"),
            Source("riscv-pk", "emu",
                "https://github.com/riscv/riscv-pk",
                "--recursive"),
            Source("riscv-isa-sim", "emu",
                "https://github.com/riscv/riscv-isa-sim",
                "--recursive"),
            Source("riscv-qemu-tests", "emu",
                "https://github.com/arsv/riscv-qemu-tests",
                "--recursive")
        ]

        commits = Commits(TOPDIR, DOCKER_DIR)
        commits.load()

        # set commits
        for src in self.srcs:
            src.commit = commits[src.name]

    # get all sources
    def get(self):
        for src in self.srcs:
            src.get()


#
# Images
#

class Image:
    commits = Commits(TOPDIR, DOCKER_DIR).load()

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


    # build if done file does not exist
    def build(self):
        # skip if done
        dir = path(DOCKER_DIR, self.name)
        done = path(dir, "done")
        if os.path.exists(done):
            return

        # docker build
        with cd(DOCKER_DIR):
            shell("docker build -t {} {}".format(
                self.img, self.name))
            shell("touch " + done)


class Images:
    build_vol = "sbt-vol-build"
    toolchain_vol = "sbt-vol-toolchain"

    def __init__(self):
        self.imgs = [
            Image("riscv-sbt"),
            Image("riscv-gnu-toolchain"),
            Image("emu",
                srcs=["riscv-fesvr",
                    "riscv-isa-sim",
                    "riscv-pk",
                    "riscv-gnu-toolchain/riscv-qemu",
                    "riscv-qemu-tests"]),
            Image("llvm", srcs=["llvm", "clang"]),
            Image("sbt", srcs=[], img="sbt"),
            Image("dev", srcs=[])
        ]
        self._iter = None


    def build(self, name):
        # build all?
        if name == "all":
            for img in self.imgs:
                img.build()
            return

        # find and build img by name
        for img in self.imgs:
            if img.name == name:
                img.build()
                break
        else:
            sys.exit("ERROR: component not found: " + args.build)


    def _find_img(self, name):
        for img in self.imgs:
            if img.name == name:
                return img
        raise Exception("Image " + name + " not found")


    def _copy_build_objs(self, img):
        if img.name in ["riscv-sbt", "dev"]:
            return

        shell("docker run --rm --mount source=" + self.build_vol +
                ",destination=/build " + img.img +
                " cp -a build /")


    def copy_build_objs(self, img_names):
        if len(img_names) == 0:
            raise Exception("No images specified")

        # create volume if needed
        try:
            shell("docker volume ls | grep " + self.build_vol)
        except:
            shell("docker volume create " + self.build_vol)

        # find images
        if len(img_names) == 1 and img_names[0] == "all":
            imgs = self.imgs
        else:
            imgs = []
            for img_name in img_names:
                imgs.append(self._find_img(img_name))

        for img in imgs:
            self._copy_build_objs(img)


    def copy_toolchain(self):
        try:
            shell("docker volume ls | grep " + self.toolchain_vol)
        except:
            shell("docker volume create " + self.toolchain_vol)

        shell("docker run --rm --mount source=" + self.toolchain_vol +
                ",destination=/toolchain sbt" +
                " cp -a toolchain /")


    def __iter__(self):
        self._iter = iter(self.imgs)
        return self._iter

#
# main
#

def is_cygwin():
    try:
        shell("uname | grep -i cygwin")
        return True
    except:
        return False


def run_dev():
    workdir = mpath(DOCKER_DIR, "riscv-sbt", "src", "riscv-sbt")
    emu = mpath(DOCKER_DIR, "emu", "src")
    llvm = mpath(DOCKER_DIR, "llvm", "src")
    vols = {
        "/riscv-sbt"            : workdir,
        "/riscv-sbt/build"      : Images.build_vol,
        "/riscv-sbt/toolchain"  : Images.toolchain_vol,
        "/riscv-sbt/submodules/riscv-gnu-toolchain" :
            mpath(DOCKER_DIR, "riscv-gnu-toolchain", "src",
                    "riscv-gnu-toolchain"),
        "/riscv-sbt/submodules/riscv-fesvr"     : path(emu, "riscv-fesvr"),
        "/riscv-sbt/submodules/riscv-isa-sim"   : path(emu, "riscv-isa-sim"),
        "/riscv-sbt/submodules/riscv-pk"        : path(emu, "riscv-pk"),
        "/riscv-sbt/submodules/riscv-qemu-tests": path(emu, "riscv-qemu-tests"),
        "/riscv-sbt/submodules/llvm"    : path(llvm, "llvm"),
        "/riscv-sbt/submodules/clang"   : path(llvm, "clang"),
    }

    def is_vol(key):
        return key in ["/riscv-sbt/build", "/riscv-sbt/toolchain"]

    if is_cygwin():
        prefix = "winpty "

        def cygpath(path):
            return shell("cygpath -m " + path, save_out=True).strip()

        for k, v in vols.items():
            if is_vol(k):
                continue
            vols[k] = cygpath(v)
    else:
        prefix = ''

    vols_str = ''
    for k, v in vols.items():
        if is_vol(k):
            t="volume"
        else:
            t="bind"
        vols_str = cat(vols_str,
                "--mount type={},source={},destination={}".format(t, v, k))

    shell(prefix + "docker run --privileged -it --rm -h dev " + vols_str + " sbt-dev")


if __name__ == "__main__":
    imgs = Images()
    names = [img.name for img in imgs]

    parser = argparse.ArgumentParser(description="docker build helper")
    parser.add_argument("--save-current-commits", action="store_true",
        help="save the current commit hash of each submodule in commits.txt")
    parser.add_argument("--get-srcs", action="store_true",
        help="clone and checkout all needed sources")
    parser.add_argument("--clean", action="store_true",
        help="remove done files from every docker build dir")
    parser.add_argument("--clean-srcs", action="store_true",
        help="remove downloaded sources dir")
    parser.add_argument("--clean-imgs", action="store_true",
        help="remove <none> and docker related images")
    parser.add_argument("--build", metavar="img", type=str,
        choices=names + ["all"],
        help="build img. imgs=[{}]".format(", ".join(names + ["all"])))
    parser.add_argument("--run", action="store_true",
        help="run the sbt container")
    parser.add_argument("--copy-build-objs",
        metavar="img", type=str, nargs='+',
        help="copy build objects from intermediate containers" +
            " (use \"all\" to copy objs from all images)")
    parser.add_argument("--copy-toolchain", action="store_true",
        help="copy toolchain files to volume")
    parser.add_argument("--dev", action="store_true")
    args = parser.parse_args()


    # --save-current-commits
    if args.save_current_commits:
        commits = Commits(TOPDIR, DOCKER_DIR)
        commits.save(commits.get_current())
    # --get-srcs
    elif args.get_srcs:
        srcs = Sources()
        srcs.get()
    # --clean
    elif args.clean:
        with cd(DOCKER_DIR):
            shell("rm -f {}".format(
                " ".join([name + "/done" for name in names])))
    # --clean-srcs
    elif args.clean_srcs:
        with cd(DOCKER_DIR):
            shell("rm -rf {}".format(
                " ".join([name + "/src" for name in names])))
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
    # --build
    elif args.build:
        imgs.build(args.build)
    # --run
    elif args.run:
        if is_cygwin():
            cmd = "winpty "
        else:
            cmd = ''
        cmd = cmd + "docker run -it --rm -h sbt sbt"
        shell(cmd)
    # --copy-build-objs
    elif args.copy_build_objs:
        imgs.copy_build_objs(args.copy_build_objs)
    # --copy-toolchain
    elif args.copy_toolchain:
        imgs.copy_toolchain()
    # --dev
    elif args.dev:
        run_dev()
    # error
    else:
        sys.exit("ERROR: no command specified")
