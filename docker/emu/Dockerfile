FROM sbt-riscv-gnu-toolchain

# qemu reqs
RUN apt-get update \
    && apt-get install -y \
    python \
    pkg-config \
    libglib2.0-dev \
    && rm -rf /var/lib/apt/lists/*

COPY src /riscv-sbt/submodules/
WORKDIR /riscv-sbt

ENV TOPDIR=/riscv-sbt
ENV PYTHONUNBUFFERED=1
ENV PYTHONPATH=/riscv-sbt/scripts
ENV PATH=$TOPDIR/toolchain/release/opt/riscv/bin:$PATH

RUN make spike \
    && rm -rf build

RUN make qemu \
    && rm -rf build

RUN rm -rf /riscv-sbt/submodules/*

CMD ["/bin/bash"]
