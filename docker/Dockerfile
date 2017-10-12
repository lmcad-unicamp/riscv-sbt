FROM debian:stretch

RUN apt-get update && \
    apt-get install -y \
    git \
    && rm -rf /var/lib/apt/lists/*

RUN git clone \
    --depth 1 -b unstable \
    https://github.com/OpenISA/riscv-sbt.git

WORKDIR /riscv-sbt

RUN git submodule init submodules/llvm
RUN git submodule update --recursive submodules/llvm

RUN git submodule init submodules/clang
RUN git submodule update --recursive submodules/clang

RUN git submodule init submodules/lowrisc-llvm
RUN git submodule update --recursive submodules/lowrisc-llvm

# RISC-V toolchain pre-reqs
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

RUN git submodule update --recursive --init submodules/riscv-gnu-toolchain

RUN ["/bin/bash", "-c", ". scripts/env.sh && make riscv-gnu-toolchain"]

RUN apt-get update \
    && apt-get install -y \
    cmake \
    ninja-build \
    python3 \
    && rm -rf /var/lib/apt/lists/*

RUN ["/bin/bash", "-c", ". scripts/env.sh && make lowrisc-llvm-debug"]

RUN apt-get update \
    && apt-get install -y \
    g++-multilib \
    && rm -rf /var/lib/apt/lists/*

RUN git pull
RUN ["/bin/bash", "-c", ". scripts/env.sh && make sbt-debug"]

RUN ["/bin/bash", "-c", ". scripts/env.sh && make riscv-gnu-toolchain-linux"]

CMD ["/bin/bash"]
