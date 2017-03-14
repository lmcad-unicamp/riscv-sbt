RISC-V Static Binary Translator
-------------------------------

How to Build
------------

Release:
```bash
source setenv.sh release
make
```
Debug:
```bash
source setenv.sh
make
```

The result files will be placed on 'toolchain' directory.

Tests
-----

There are two kinds of tests: unit tests, that automatically check and report errors,
and other tests, whose output must be manually verified. But even for unchecked tests,
if they compile and run without errors/crashes, it is already a good signal.

The rv32-system test requires enabling RDPMC fixed counters in user mode to be able to emulate
the RDINSTRET CSR correctly:
```bash
sudo modprobe msr
sudo scripts/setmsr.sh
```

Running the unit tests:
```bash
make -C sbt/tests run-tests
```

Running all tests:
```bash
make tests
```
