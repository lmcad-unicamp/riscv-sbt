FROM debian:buster

RUN apt-get update \
    && apt-get install -y \
    autoconf \
    automake \
    autotools-dev \
    bc \
    bison \
    build-essential \
    cmake \
    clang-6.0 \
    curl \
    device-tree-compiler \
    flex \
    gawk \
    gcc-7-i686-linux-gnu \
    gcc-7-arm-linux-gnueabihf \
    g++-7-i686-linux-gnu \
    git \
    gperf \
    libexpat-dev \
    libgmp-dev \
    libmpc-dev \
    libmpfr-dev \
    libncursesw5-dev \
    libtool \
    linux-perf \
    ninja-build \
    patchutils \
    python3 \
    python3-numpy \
    texinfo \
    vim \
    zlib1g-dev \
    && rm -rf /var/lib/apt/lists/*

RUN dpkg --add-architecture i386 \
    && apt-get update \
    && apt-get install -y \
    libunwind8:i386 \
    linux-perf-4.18:i386 \
    && rm -rf /var/lib/apt/lists/*

ENV TOPDIR=/riscv-sbt
ENV PYTHONUNBUFFERED=1
ENV PYTHONPATH=/riscv-sbt/scripts
ENV MAKE_OPTS=-j2

ENV PATH=$TOPDIR/toolchain/release/opt/riscv/bin:$PATH
ENV PATH=$TOPDIR/toolchain/release/bin:$PATH
ENV PATH=$TOPDIR/toolchain/debug/bin:$PATH
ENV PATH=$TOPDIR/toolchain/release/gcc7/opt/riscv/bin:$PATH
ENV PATH=$TOPDIR/toolchain/debug/gcc7/bin:$PATH

COPY vimrc /root/.vimrc

RUN echo "alias ls='ls --color'" >> /root/.bashrc \
    && echo "echo 0 > /proc/sys/kernel/kptr_restrict" >> /root/.bashrc \
    && echo "source scripts/env.sh" >> /root/.bashrc

WORKDIR /riscv-sbt
CMD ["/bin/bash"]
