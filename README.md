RISC-V Static Binary Translator (SBT)
-------------------------------------

How to Build
------------

To build the SBT and the RISC-V toolchain, use the following:

```bash
. scripts/env.sh
make
```

The result files will be placed on 'toolchain' directory.

Note that several tools are required to build the SBT.
The best way to find them is to look at the Dockerfiles, in docker dir.


Docker build
------------

Alternatively, a docker image can be built and used instead:

```bash
make docker-img
```

In this case, the docker build will take care of the installation of all
pre-requisites.
The host machine needs to have only these tools installed:
- docker-ce
- git
- make
- python3


MiBench
-------

To build MiBench benchmarks, use the following commands:

```bash
. scripts/env.sh
cd mibench
./genmake.py
make
```

This will build all supported benchmarks, for RISC-V, x86, and ARM, along with
the translated RISC-V to x86 and RISC-V to ARM binaries. If you don't want to
build ARM binaries, pass --no-arm to genmake.py.

To check if the translated binaries produce the same results as the native
ones, use `make benchs-test`. Note that, except for RISC-V binaries, that will
be run with QEMU, non-native binaries will not be run, for obvious reasons.
Thus, to test ARM binaries you need to be on an ARM host.

To measure the execution times of the benchmarks, first make sure that regular
users have permission to use Perf, or use the alias 'perfperm' to set it
correctly.  Then, use `make benchs-measure` to measure the times of all
binaries.

To run MiBench on an ARM host, the fastest way (and currently the only
        supported one) is to cross compile the ARM binaries on an x86 host and
copy the binaries to the ARM host through SSH. By default, `./genmake.py &&
make` already builds all ARM binaries. To copy them to the remote machine,
    first adjust ARM and ARM_TOPDIR to point the desired target, then use
    `make benchs-arm-copy` to copy them with SSH. To run then, SSH to the ARM
    machine, and follow the same steps as above. Note that on the ARM machine
    only the riscv-sbt files are needed, as no submodules are used to run the
    benchmarks on ARM.


Unit Tests
----------

There are a couple of unit tests written to check if the toolchain and RISC-V
are working correctly, that may be useful when changing the SBT or hunting
bugs. To run them, use:

```bash
. scripts/env.sh
make almost-alltests
```

There is also an 'alltests' target, but includes a test for mostly unused
features of the SBT, such as (limited) ecall translation and CSR reads of
time, cycles, and instructions retired, and it can be tricky to get it working
properly.
