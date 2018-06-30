FROM sbt-riscv-sbt

RUN apt-get update \
    && apt-get install -y \
    autoconf \
    automake \
    autotools-dev \
    bison \
    build-essential \
    device-tree-compiler \
    flex \
    libglib2.0-dev \
    libtool \
    pkg-config \
    python \
    && rm -rf /var/lib/apt/lists/*

ENV PATH=$TOPDIR/toolchain/release/opt/riscv/bin:$PATH

CMD ["/bin/bash"]
