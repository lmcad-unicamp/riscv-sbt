FROM sbt-riscv-gnu-toolchain

# llvm/clang reqs
RUN apt-get update \
    && apt-get install -y \
    cmake \
    g++-multilib \
    ninja-build \
    && rm -rf /var/lib/apt/lists/*

COPY src /riscv-sbt/submodules/llvm/
WORKDIR /riscv-sbt

ENV TOPDIR=/riscv-sbt
ENV PYTHONUNBUFFERED=1
ENV PYTHONPATH=/riscv-sbt/scripts
ENV PATH=$TOPDIR/toolchain/release/opt/riscv/bin:$PATH

RUN make llvm \
    && rm -rf build

CMD ["/bin/bash"]
