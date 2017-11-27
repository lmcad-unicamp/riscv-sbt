RISC-V Static Binary Translator (SBT)
-------------------------------------

How to Build
------------

```bash
. scripts/env.sh
make
```

The result files will be placed on 'toolchain' directory.

Note that several tools are required to build the SBT.
The best place to find them is to look at docker/Dockerfile.


Docker build
------------

Alternatively, a docker image can be built and used instead:

```bash
make -C docker
```

In this case, the Dockerfile takes care of all build pre-requisites.
