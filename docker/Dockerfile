FROM debian:stretch

# install git and unbuffer (expect) to perform the clone, showing
# its progress
RUN apt-get update && \
    apt-get install -y \
    expect \
    git \
    && rm -rf /var/lib/apt/lists/*

# clone main repo
RUN unbuffer git clone --recursive https://github.com/OpenISA/riscv-sbt.git

WORKDIR /riscv-sbt

# install RISC-V toolchain pre-reqs
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

# riscv-gnu-toolchain
RUN /bin/bash -c \
    ". scripts/env.sh && make riscv-gnu-toolchain && rm -rf build"
RUN /bin/bash -c \
    ". scripts/env.sh && make riscv-gnu-toolchain-linux && rm -rf build"

# LLVM/clang
RUN apt-get update \
    && apt-get install -y \
    cmake \
    g++-multilib \
    ninja-build \
    python3 \
    && rm -rf /var/lib/apt/lists/*

RUN /bin/bash -c \
    ". scripts/env.sh && make MAKE_OPTS=-j4 lowrisc-llvm-debug && rm -rf build"

# SBT
RUN /bin/bash -c \
    ". scripts/env.sh && make sbt-debug && rm -rf build"

# spike
RUN /bin/bash -c \
    ". scripts/env.sh && make riscv-isa-sim && rm -rf build"
RUN /bin/bash -c \
    ". scripts/env.sh && make riscv-pk-32 && rm -rf build"

# qemu
RUN apt-get update \
    && apt-get install -y \
    python \
    pkg-config \
    libglib2.0-dev \
    && rm -rf /var/lib/apt/lists/*

RUN /bin/bash -c \
    ". scripts/env.sh && make qemu-user && rm -rf build"

# needed by test-system
#RUN apt-get update \
#    && apt-get install -y \
#    msr-tools \
#    && rm -rf /var/lib/apt/lists/*
#
#RUN ["/bin/bash", "-c",
# ". scripts/env.sh && ./scripts/setmsr.sh && make test-system"]

CMD ["/bin/bash"]
