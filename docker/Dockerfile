FROM debian:stretch

# install git and unbuffer (expect) to perform the clone, showing
# its progress
RUN apt-get update && \
    apt-get install -y \
    expect \
    git \
    && rm -rf /var/lib/apt/lists/*


### get sources ###

# clone main repo
RUN unbuffer git clone https://github.com/OpenISA/riscv-sbt.git
WORKDIR /riscv-sbt
RUN git checkout aae63754a71617ea6ba8cfee4a42288a97de7eab

# get riscv-gnu-toolchain
RUN cd submodules \
    && unbuffer git clone --recursive https://github.com/riscv/riscv-gnu-toolchain
RUN cd submodules/riscv-gnu-toolchain \
    && git checkout bf5697a1a6509705b50dcc1f67b8c620a7b21ec4

# get llvm
RUN cd submodules \
    && unbuffer git clone --recursive -b lowrisc \
       https://github.com/OpenISA/llvm
RUN cd submodules/llvm \
    && git checkout 5c76c41588f9816dc750c7be565a1f109b39c72f

# get clang
RUN cd submodules \
    && unbuffer git clone --recursive -b lowrisc \
       https://github.com/OpenISA/clang
RUN cd submodules/clang \
    && git checkout 1e850113a8799cf1aa5a8b9e9fa6d8181f96b8d2

# get fesvr
RUN cd submodules \
    && unbuffer git clone --recursive https://github.com/riscv/riscv-fesvr
RUN cd submodules/riscv-fesvr \
    && git checkout f683e01542acf60e50774d061bcb396b628e3e67

# get proxy kernel
RUN cd submodules \
    && unbuffer git clone --recursive https://github.com/riscv/riscv-pk
RUN cd submodules/riscv-pk \
    && git checkout 18087efa98d77918d55127c3d7745cd6d6d9d77b

# get spike
RUN cd submodules \
    && unbuffer git clone --recursive https://github.com/riscv/riscv-isa-sim
RUN cd submodules/riscv-isa-sim \
    && git checkout 3b1e9ab7522b3b20cde6bd8d9f2b28222463cf1b

# get qemu-tests
RUN cd submodules \
    && unbuffer git clone --recursive https://github.com/arsv/riscv-qemu-tests
RUN cd submodules/riscv-qemu-tests \
    && git checkout 92f007e7c145976dbf64de43d99679461272d108


### install pre-reqs ###

# riscv-gnu-toolchain reqs
RUN apt-get update \
    && apt-get install -y \
    autoconf \
    automake \
    autotools-dev \
    bc \
    bison \
    build-essential \
    curl \
    device-tree-compiler \
    flex \
    gawk \
    gperf \
    libgmp-dev \
    libmpc-dev \
    libmpfr-dev \
    libtool \
    patchutils \
    texinfo \
    zlib1g-dev \
    && rm -rf /var/lib/apt/lists/*

# llvm/clang reqs
RUN apt-get update \
    && apt-get install -y \
    cmake \
    g++-multilib \
    ninja-build \
    python3 \
    && rm -rf /var/lib/apt/lists/*

# qemu reqs
RUN apt-get update \
    && apt-get install -y \
    python \
    pkg-config \
    libglib2.0-dev \
    && rm -rf /var/lib/apt/lists/*


### build srcs ###

# XXX
#RUN git pull

# build riscv-gnu-toolchain - newlib version
RUN /bin/bash -c \
    ". scripts/env.sh && make riscv-gnu-toolchain && rm -rf build"
# build riscv-gnu-toolchain - linux version
RUN /bin/bash -c \
    ". scripts/env.sh && make riscv-gnu-toolchain-linux && rm -rf build"

# build llvm/clang
RUN /bin/bash -c \
    ". scripts/env.sh && make MAKE_OPTS=-j4 lowrisc-llvm-debug && rm -rf build"

# build fesvr & spike
RUN /bin/bash -c \
    ". scripts/env.sh && make riscv-isa-sim && rm -rf build"

# build pk
RUN /bin/bash -c \
    ". scripts/env.sh && make riscv-pk-32 && rm -rf build"

# build qemu
RUN /bin/bash -c \
    ". scripts/env.sh && make qemu-user && rm -rf build"


### SBT ###

# system-test
# DISABLED: msr.ko not easily available
#
# system-test reqs
#RUN apt-get update \
#    && apt-get install -y \
#    kmod \
#    msr-tools \
#    sudo \
#    && rm -rf /var/lib/apt/lists/*

### XXX: time saving hacks! (remove later) ###
# BEGIN hack
#RUN git pull
#ADD make3/ make/
##ADD make/Makefile .
# END hack

# build sbt
RUN /bin/bash -c \
    ". scripts/env.sh && make sbt-debug && rm -rf build"

# run basic tests
RUN /bin/bash -c \
    ". scripts/env.sh && mkdir -p junk && make tests && rm -rf build"

# system-test
#RUN /bin/bash -c \
#    ". scripts/env.sh && make system-test && rm -rf build"

# qemu-tests
RUN /bin/bash -c \
    ". scripts/env.sh && make rv32tests-run rv32tests-clean"

CMD ["/bin/bash"]
