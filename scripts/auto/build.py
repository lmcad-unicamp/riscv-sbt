#!/usr/bin/env python3

from auto.config import ARCH, GOPTS, RV32, RV32_LINUX, \
                        SBT, TOOLS, X86, emit_llvm
from auto.utils import cat, cd, chsuf, path, shell

import argparse
import os
import re
import sys

class BuildOpts:
    def __init__(self):
        # hard coded options
        self.setsysroot = True


    @staticmethod
    def parse(args):
        opts = BuildOpts()

        opts.arch = ARCH[args.arch]
        opts.srcdir = args.srcdir
        opts.dstdir = args.dstdir
        opts.ins = args.ins
        opts.first = args.ins[0]
        opts.out = args.o if args.o else chsuf(opts.first, '')

        opts.clink = args.C
        opts.asm = opts.first.endswith(".s")
        opts.obj = opts.out.endswith(".o")

        opts.cc = args.cc
        if args._as:
            opts._as = args._as
        elif opts.cc == "gcc":
            opts._as = "as"
        else:
            opts._as = "mc"
        opts.cflags = args.cflags
        opts.gccflags = args.gccflags
        opts.arch_gccflags = {}
        if args.gccflags_x86:
            opts.arch_gccflags[X86.prefix] = args.gccflags_x86
        opts.oflags = args.oflags
        opts.llcflags = {}
        if args.llcflags_x86:
            opts.llcflags[X86.prefix] = args.llcflags_x86
        opts.sflags = args.sflags
        opts.ldflags = cat(*[SBT.nat_obj(opts.arch, o, opts.clink)
            for o in args.sbtobjs])
        opts.sbtflags = cat(" ".join(args.sbtflags).strip(),
            "-dont-use-libc" if not opts.clink else "")
        opts.dbg = args.dbg
        opts.opt = True if args.opt or not args.dbg else False
        opts.relax = not args.no_relax

        return opts


    @staticmethod
    def add_to_parser(parser):
        parser.add_argument("--arch", default="rv32")
        parser.add_argument("--srcdir", default=".")
        parser.add_argument("--dstdir", default=".")
        parser.add_argument("ins", nargs="+", metavar="in")
        parser.add_argument("-o", help="default: 1st in without suffix")

        parser.add_argument("-C", action='store_false',
            help="don't link with C libs")
        parser.add_argument("--sbtobjs", nargs="+",
            default=["syscall", "runtime"],
            help="optional SBT native objs to link with")

        parser.add_argument("--cc", help="compiler to use",
            choices=["clang", "gcc"], default="clang")
        parser.add_argument("--as", dest="_as",
            choices=["as", "mc"], help="assembler to use")
        parser.add_argument("--cflags", help="compiler flags")
        parser.add_argument("--gccflags", help="gcc compiler flags")
        parser.add_argument("--gccflags-x86", help="gcc compiler flags for x86")
        parser.add_argument("--oflags", help="opt flags")
        parser.add_argument("--llcflags-x86", help="llc flags for x86")
        parser.add_argument("--sflags", help="assembler flags")
        parser.add_argument("--ldflags", help="linker flags")
        parser.add_argument("--sbtflags", nargs="+", metavar="flag",
            help="translator flags", default=[])
        parser.add_argument("--dbg", action='store_true',
            help="build for debug")
        parser.add_argument("--opt", action='store_true',
            help="optmize code, even if debug is enabled")
        parser.add_argument("--no-relax", action='store_true',
            help="disable relax")


class LLVMBuilder:
    def __init__(self, opts):
        self.opts = opts


    def _c2bc(self, srcdir, dstdir, _in, out):
        """ .c -> .bc """
        opts = self.opts
        arch = opts.arch
        flags = cat(
            arch.clang_flags,
            (arch.sysroot_flag if opts.setsysroot else ""),
            opts.cflags,
            emit_llvm(opts.dbg, opts.opt))

        ipath = path(srcdir, _in)
        opath = path(dstdir, out)

        cmd = cat(arch.clang, flags, ipath, "-o", opath)
        shell(cmd)


    def _lllink(self, dir, ins, out):
        """ *.bc -> .bc """
        ipaths = [path(dir, i) for i in ins]
        opath = path(dir, out)

        cmd = cat(TOOLS.link, " ".join(ipaths), "-o", opath)
        shell(cmd)


    def dis(self, dir, mod):
        """ .bc -> .ll """
        ipath = path(dir, mod)
        opath = path(dir, chsuf(mod, ".ll"))

        cmd = cat(TOOLS.dis, ipath, "-o", opath)
        shell(cmd)


    def opt(self, dir, _in, out, printf_break):
        """ opt """
        opts = self.opts
        ipath = path(dir, _in)
        opath = path(dir, out)

        flags = cat(TOOLS.opt_flags(opts.opt), opts.oflags)
        if printf_break:
            flags = cat(flags,
                "-load", "libPrintfBreak.so", "-printf-break")

        cmd = cat(TOOLS.opt, flags, ipath, "-o", opath)
        shell(cmd)


    def _c2lbc(self, srcdir, dstdir, ins, out):
        """ *.c -> *.bc -> .bc
        (C2LBC: C to linked BC)
        """
        ch2bc1 = lambda c: chsuf(c, '') + "_bc1.bc"

        if len(ins) == 1:
            self._c2bc(srcdir, dstdir, ins[0], out)
        else:
            for c in ins:
                o = ch2bc1(c)
                self._c2bc(srcdir, dstdir, c, o)
                self.dis(dstdir, o)
            self._lllink(dstdir, [ch2bc1(c) for c in ins], out)
        self.dis(dstdir, out)


    def bc2s(self, dir, _in, out):
        """ .bc -> .s """
        opts = self.opts
        arch = opts.arch
        ipath = path(dir, _in)
        opath = path(dir, out)
        flags = "-O3" if opts.opt else "-O0"
        if arch.prefix in opts.llcflags.keys():
            flags = cat(flags, opts.llcflags[arch.prefix])
        else:
            flags = cat(flags, arch.llcflags)

        cmd = cat(arch.llc, flags, ipath, "-o", opath)
        shell(cmd)
        # workaround for align issue with as
        if opts._as == "as":
            if arch == RV32 or arch == RV32_LINUX:
                cmd = 'sed -i "1i.option norelax" ' + opath
                shell(cmd)


    def c2s(self, srcdir, dstdir, ins, out):
        """ *.c -> .s """

        bc = chsuf(out, ".bc")
        self._c2lbc(srcdir, dstdir, ins, bc)

        if not self.opts.opt:
            self.opt(dstdir, bc, bc, printf_break=GOPTS.printf_break)
            self.dis(dstdir, bc)
            self.bc2s(dstdir, bc, out)

        else:
            opt1 = chsuf(bc, ".opt.bc")
            opt2 = chsuf(bc, ".opt2.bc")

            # opt; dis; opt; dis; .bc -> .s
            self.opt(dstdir, bc, opt1, printf_break=GOPTS.printf_break)
            self.dis(dstdir, opt1)
            self.opt(dstdir, opt1, opt2, printf_break=False)
            self.dis(dstdir, opt2)
            self.bc2s(dstdir, opt2, out)


class GCCBuilder:
    def __init__(self, opts):
        self.opts = opts

    def _c2u(self, srcdir, dstdir, ins, out):
        with open(path(dstdir, out), "w") as fout:
            for src in ins:
                ln = '#include "{0}"\n'.format(path(srcdir, src))
                fout.write(ln)


    def c2s(self, srcdir, dstdir, ins, out):
        opts = self.opts
        arch = opts.arch

        u = chsuf(out, ".c")
        self._c2u(srcdir, dstdir, ins, u)

        flags = arch.gccflags
        if opts.dbg:
            flags = cat(flags, "-g")
        if opts.opt:
            flags = cat(flags, "-O3")
        else:
            flags = cat(flags, "-O0")
        flags = cat(flags, opts.cflags, opts.gccflags)
        if arch.prefix in opts.arch_gccflags.keys():
            flags = cat(flags, opts.arch_gccflags[arch.prefix])
        else:
            flags = cat(flags, arch.gccoflags)

        cmd = cat(arch.gcc, flags,
                path(dstdir, u), "-S", "-o", path(dstdir, out))
        shell(cmd)


    def merge_text(self, dstdir, obj):
        opts = self.opts
        arch = opts.arch

        if arch != RV32_LINUX:
            raise Exception("unexpected arch")

        old = chsuf(obj, ".old.o")
        ipath = path(dstdir, old)
        opath = path(dstdir, obj)
        shell("mv {} {}".format(opath, ipath))

        cmd = cat(arch.ld, arch.ld_flags, "-r {} -o {} -T {}")
        cmd = cmd.format(ipath, opath, path(SBT.share_dir, "elf32lriscv.x"))
        shell(cmd)


class Builder:
    def __init__(self, opts):
        self.opts = opts
        arch = opts.arch
        if opts.cc == "clang":
            self.bldr = LLVMBuilder(opts)
        else:
            self.bldr = GCCBuilder(opts)


    def _s2o(self, srcdir, dstdir, _in, out):
        """ .s -> .o """

        opts = self.opts
        arch = opts.arch
        ipath = path(srcdir, _in)
        opath = path(dstdir, out)

        if opts._as == "as":
            flags = cat(arch.as_flags, opts.sflags,
                "-g" if opts.dbg else '')
            cmd = cat(arch._as, flags, "-c", ipath, "-o", opath)
            shell(cmd)

        # llvm-mc
        # Unlike GNU as for RISC-V, llvm-mc don't screw up with alignment in the
        # generated object file, that would otherwise be fixed only later,
        # at link stage, after it's done with relaxing.
        else:
            mc = TOOLS.mc

            # preserve rv32/x86 source code for debug, by not mixing in
            # assembly source together
            if (opts.dbg and
                (out.startswith('rv32-x86-') or
                out.startswith('rv32-arm-'))):
                flags = "-g"
                # workaround for different llvm-mc behavior of newer versions
                # for some reason it gets confused by the .file directive now
                # which makes it skip generation of debug info
                shell('sed -i "/\t.file/d" ' + ipath)
            else:
                flags = ""

            flags = cat(
                flags,
                opts.sflags,
                "-arch=" + arch.march,
                "-mattr=" + arch.mattr(opts.relax))
            cmd = cat(mc, flags,
                "-assemble", "-filetype=obj",
                ipath, "-o", opath)
            shell(cmd)
            if arch.is_rv32() and GOPTS.rv_hard_float():
                self.set_abi_hf(opath, opath)

        if arch.is_rv32() and opts.cc == "gcc":
            self.bldr.merge_text(dstdir, out)


    def set_abi_hf(self, ipath, opath):
        # temporary workaround: set double-float ABI flag in ELF object file
        if ipath != opath:
            shell("cp {} {}".format(ipath, opath))
        shell(R"printf '\x04\x00\x00\x00' | " +
            "dd of=" + opath + " bs=1 seek=$((0x24)) count=4 conv=notrunc" +
            " >/dev/null 2>&1")


    def _link(self, srcdir, dstdir, ins, out):
        """ *.o -> bin """

        opts = self.opts
        arch = opts.arch
        if opts.clink:
            tool = arch.gcc
            flags = arch.gccflags
            if opts.dbg:
                flags = cat(flags, "-g")
            if opts.opt:
                flags = cat(flags, "-O3")
            else:
                flags = cat(flags, "-O0")
        else:
            tool = arch.ld
            flags = arch.ld_flags
        if GOPTS.static:
            flags = cat(flags, "-static")
        flags = cat(flags, opts.ldflags)
        if opts.clink:
            flags = cat(flags, "-lm")

        ipaths = [path(srcdir, i) for i in ins]
        opath = path(dstdir, out)
        cmd = cat(tool, " ".join(ipaths), "-o", opath, flags)
        shell(cmd)



    def _snlink(self):
        """ *.s -> bin """

        opts = self.opts
        if len(opts.ins) == 1:
            obj = opts.out + ".o"
            self._s2o(opts.srcdir, opts.dstdir, opts.ins[0], obj)
            self._link(opts.dstdir, opts.dstdir, [obj], opts.out)
        else:
            for i in ins:
                self._s2o(opts.srcdir, opts.dstdir, i, chsuf(i, ".o"))
            self._link(opts.dstdir, opts.dstdir,
                [_chsuf(i, ".o") for i in opts.ins], opts.out)


    def _cnlink(self):
        """ *.c -> bin """

        opts = self.opts
        s = opts.out + ".s"
        o = opts.out + ".o"

        self.bldr.c2s(opts.srcdir, opts.dstdir, opts.ins, s)
        self._s2o(opts.dstdir, opts.dstdir, s, o)
        self._link(opts.dstdir, opts.dstdir, [o], opts.out)


    def _gen_list(self):
        opts = self.opts
        ipath = path(opts.dstdir, opts.out + ".o")
        ofile = opts.out + ".list"
        opath = path(opts.dstdir, ofile)

        cmd = cat(TOOLS.objdump, "-S", ipath, ">", opath)
        shell(cmd)

        self.addr2src(ofile, opts.out)


    def build(self):
        """ *.c/*.s -> bin """

        opts = self.opts
        # create dstdir if needed
        shell("mkdir -p " + opts.dstdir)

        if opts.obj:
            if opts.asm:
                if len(opts.ins) > 1:
                    raise Exception("Can't generate .o for more than 1 .s")
                self._s2o(opts.srcdir, opts.dstdir, opts.ins[0], opts.out)
            else:
                s = chsuf(opts.out, '.s')
                self.bldr.c2s(opts.srcdir, opts.dstdir, opts.ins, s)
                self._s2o(opts.dstdir, opts.dstdir, s, out)
            return

        if opts.asm:
            self._snlink()
        else:
            self._cnlink()

        if opts.dbg and opts.arch == RV32_LINUX:
            self._gen_list()


    def addr2src(self, _in, out):
        opts = self.opts

        with open(path(opts.dstdir, _in)) as fin, \
            open(path(opts.dstdir, out + ".a2s"), "w") as fout:
                patt = re.compile('([ 0-9a-f]{4,8}):\t')
                lns = []
                for ln in fin:
                    m = patt.match(ln)
                    if m:
                        if lns:
                            addr = int(m.group(1), 16)
                            # print("[{:04X}]:".format(addr))
                            fout.write("[{:04X}]:\n".format(addr))
                            for l in lns:
                                #print(l, end='')
                                fout.write(l)
                            lns = []
                    else:
                        lns.append(ln)


### main ###

if __name__ == "__main__":
    # get args
    parser = argparse.ArgumentParser(description="build program")
    parser.add_argument("--addr2src", action='store_true')
    parser.add_argument("--set-abi-hf", action='store_true')
    BuildOpts.add_to_parser(parser)
    args = parser.parse_args()

    opts = BuildOpts.parse(args)
    bld = Builder(opts)

    # addr2src
    if args.addr2src:
        bld.addr2src(args.ins[0], args.o)
    elif args.set_abi_hf:
        bld.set_abi_hf(opts.first, args.o)
    # build
    else:
        bld.build()
