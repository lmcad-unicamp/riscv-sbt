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
