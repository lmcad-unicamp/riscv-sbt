FROM sbt-emu as emu

FROM debian:stretch
WORKDIR /riscv-sbt
COPY --from=emu /riscv-sbt .

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
    python3 \
    && rm -rf /var/lib/apt/lists/*

COPY src /riscv-sbt/submodules/

ENV TOPDIR=/riscv-sbt
ENV PYTHONUNBUFFERED=1
ENV PYTHONPATH=/riscv-sbt/scripts
ENV PATH=$TOPDIR/toolchain/release/opt/riscv/bin:$PATH
ENV MAKE_OPTS=-j2

RUN make llvm \
    && rm -rf build

RUN rm -rf /riscv-sbt/submodules/*

CMD ["/bin/bash"]
