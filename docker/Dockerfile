FROM debian:stretch

# install git and unbuffer (expect) to perform the clone, showing
# its progress
RUN apt-get update && \
    apt-get install -y \
    expect \
    git \
    && rm -rf /var/lib/apt/lists/*

# clone main repo
RUN unbuffer git clone https://github.com/OpenISA/riscv-sbt.git

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
RUN unbuffer git submodule update --recursive --init submodules/riscv-gnu-toolchain
RUN ["/bin/bash", "-c", ". scripts/env.sh && make riscv-gnu-toolchain"]
RUN ["/bin/bash", "-c", ". scripts/env.sh && make riscv-gnu-toolchain-linux"]

# LLVM/clang
RUN unbuffer git submodule update --recursive --init submodules/llvm
RUN unbuffer git submodule update --recursive --init submodules/clang
RUN unbuffer git submodule update --recursive --init submodules/lowrisc-llvm

RUN apt-get update \
    && apt-get install -y \
    cmake \
    g++-multilib \
    ninja-build \
    python3 \
    && rm -rf /var/lib/apt/lists/*

RUN ["/bin/bash", "-c", ". scripts/env.sh && make MAKE_OPTS=-j4 lowrisc-llvm-debug"]

# SBT
RUN ["/bin/bash", "-c", ". scripts/env.sh && make sbt-debug"]

# riscv emulators
RUN git submodule update --recursive --init submodules/riscv-fesvr
RUN git submodule update --recursive --init submodules/riscv-isa-sim
RUN git submodule update --recursive --init submodules/riscv-pk

RUN ["/bin/bash", "-c", ". scripts/env.sh && make riscv-isa-sim"]
RUN ["/bin/bash", "-c", ". scripts/env.sh && make riscv-pk-32"]

# qemu
RUN git submodule update --recursive --init submodules/riscv-qemu

RUN apt-get update \
    && apt-get install -y \
    python \
    pkg-config \
    libglib2.0-dev \
    && rm -rf /var/lib/apt/lists/*

RUN ["/bin/bash", "-c", ". scripts/env.sh && make qemu-user"]

CMD ["/bin/bash"]
