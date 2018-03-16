FROM sbt-riscv-gnu-toolchain as toolchain

FROM sbt-riscv-sbt
WORKDIR /riscv-sbt
COPY --from=toolchain /riscv-sbt/toolchain /riscv-sbt/toolchain
COPY --from=toolchain /riscv-sbt/submodules/riscv-gnu-toolchain/riscv-qemu \
            /riscv-sbt/submodules/riscv-gnu-toolchain/riscv-qemu

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

COPY src /riscv-sbt/submodules/

ENV TOPDIR=/riscv-sbt
ENV PYTHONUNBUFFERED=1
ENV PYTHONPATH=/riscv-sbt/scripts
ENV PATH=$TOPDIR/toolchain/release/opt/riscv/bin:$PATH

RUN make spike
RUN make qemu

CMD ["/bin/bash"]
