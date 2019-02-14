RISC-V Static Binary Translator (SBT)
-------------------------------------

How to Build
------------

To build the SBT and the RISC-V toolchain, use the following:

```bash
git submodule update --init --recursive
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
copy them to the ARM host through SSH. By default, `./genmake.py &&
make` already builds all ARM binaries. To copy them to the remote machine,
first adjust ARM and ARM_TOPDIR environment variables to point the desired
target, then use `make benchs-arm-copy` to copy them with SSH. To run them,
SSH to the ARM machine and follow the same steps as above. Note that on the
ARM machine only the riscv-sbt files are needed, as no submodules are used
to run the benchmarks on ARM.


MiBench on Docker
-----------------

Alternatively, Docker can be used to build and run MiBench instead, but only
for x86. Use the following commands to build, test and run MiBench:

```bash
make docker-mibuild
make docker-mitest
make docker-mirun
```

The results will be available at mibench/mibench.csv. Use the measure.py
script to print the results in a more readable format, or check how to plot a
graph from it in the next section.


Graph Plotting
--------------

Briefly, to update the performance and comparison graphs with the results of a new MiBench
run, do the following:

- copy the resulting mibench.csv file to results/&lt;run-name&gt;.csv
- in results dir, run `./plot.sh`
- use `./plot.sh clean` or remove the corresponding intermediate files to force the generation of a new graph

Here, &lt;run-name&gt; should be the name of one of the already present .csv files in results dir,
excluding the *_slowdown.csv ones. The name is related to the settings used in the run. For instance,
x86-avx-gcchf means that the benchmarks were run on an x86 host, with AVX extensions enabled, using GCC with
hard-float ABI to build RISC-V and native binaries. Most of the settings can be adjusted at scripts/auto/config.py
file.

Unit Tests
----------

There are a couple of unit tests written to check if the toolchain and RISC-V SBT
are working correctly, that may be useful when changing the SBT or hunting
bugs. To run them, use:

```bash
. scripts/env.sh
make almost-alltests
```

There is also an 'alltests' target, but it includes a test for mostly unused
features of the SBT, such as (limited) ecall translation and CSR reads of
time, cycles, and instructions retired. Also, it can be tricky to get it working
properly.
