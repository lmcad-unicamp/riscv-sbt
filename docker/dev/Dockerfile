FROM sbt

WORKDIR /riscv-sbt
ENV TOPDIR=/riscv-sbt
ENV PYTHONUNBUFFERED=1
ENV PYTHONPATH=/riscv-sbt/scripts
ENV PATH=$TOPDIR/toolchain/release/opt/riscv/bin:$PATH
ENV PATH=$TOPDIR/toolchain/release/bin:$PATH
ENV PATH=$TOPDIR/toolchain/debug/bin:$PATH
ENV MAKE_OPTS=-j2

VOLUME /riscv-sbt/build
VOLUME /riscv-sbt/sbt
VOLUME /riscv-sbt/scripts
VOLUME /riscv-sbt/submodules

RUN apt-get update \
    && apt-get install -y \
    gdb \
    vim \
    && rm -rf /var/lib/apt/lists/*

RUN echo "alias ls='ls --color'" >> /root/.bashrc

CMD ["/bin/bash"]
