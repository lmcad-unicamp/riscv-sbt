FROM debian:stretch

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


### set env ###

ENV TOPDIR=/riscv-sbt
VOLUME $TOPDIR
WORKDIR $TOPDIR

ENV PYTHONPATH=$TOPDIR/scripts
ENV PYTHONUNBUFFERED=1
ENV PATH=$TOPDIR/toolchain/release/opt/riscv/bin:$PATH

ENTRYPOINT ["make"]
