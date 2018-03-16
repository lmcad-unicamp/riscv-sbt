FROM sbt-emu as emu

FROM sbt-riscv-sbt
WORKDIR /riscv-sbt
COPY --from=emu /riscv-sbt/toolchain /riscv-sbt/toolchain

# llvm/clang reqs
# clang-3.9: to build LLVM (gcc could be used instead)
RUN apt-get update \
    && apt-get install -y \
    build-essential \
    cmake \
    clang-3.9 \
    g++-multilib \
    ninja-build \
    python \
    && rm -rf /var/lib/apt/lists/*

COPY src /riscv-sbt/submodules/

ENV TOPDIR=/riscv-sbt
ENV PYTHONUNBUFFERED=1
ENV PYTHONPATH=/riscv-sbt/scripts
ENV PATH=$TOPDIR/toolchain/release/opt/riscv/bin:$PATH
ENV MAKE_OPTS=-j2

RUN make llvm

CMD ["/bin/bash"]
