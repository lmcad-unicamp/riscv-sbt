FROM sbt-riscv-sbt

# riscv-gnu-toolchain
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
    libexpat-dev \
    libgmp-dev \
    libmpc-dev \
    libmpfr-dev \
    libncursesw5-dev \
    libtool \
    patchutils \
    texinfo \
    zlib1g-dev \
    && rm -rf /var/lib/apt/lists/*

ENV TOPDIR=/riscv-sbt
ENV PYTHONUNBUFFERED=1
ENV PYTHONPATH=/riscv-sbt/scripts
ENV MAKE_OPTS=-j2

WORKDIR /riscv-sbt
CMD ["/bin/bash"]
