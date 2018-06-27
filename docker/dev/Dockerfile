FROM debian:stretch

WORKDIR /riscv-sbt
ENV TOPDIR=/riscv-sbt
ENV PYTHONUNBUFFERED=1
ENV PYTHONPATH=/riscv-sbt/scripts
ENV PATH=$TOPDIR/toolchain/release/opt/riscv/bin:$PATH
ENV PATH=$TOPDIR/toolchain/release/bin:$PATH
ENV PATH=$TOPDIR/toolchain/debug/bin:$PATH
ENV MAKE_OPTS=-j2

RUN dpkg --add-architecture i386 \
    && apt-get update \
    && apt-get install -y \
    build-essential \
    clang-3.9 \
    cmake \
    device-tree-compiler \
    g++-multilib \
    gdb \
    git \
    libglib2.0-0 \
    libunwind8:i386 \
    linux-headers-amd64 \
    linux-perf-4.9:i386 \
    man \
    ninja-build \
    vim \
    python3 \
    python3-numpy \
    && rm -rf /var/lib/apt/lists/*

RUN sed -i "1s/main/& contrib non-free/" /etc/apt/sources.list \
    && apt-get update \
    && apt-get install -y \
    gcc-doc \
    gcc-6-arm-linux-gnueabihf \
    libc6-dev:i386 \
    && rm -rf /var/lib/apt/lists/*

RUN apt-get update \
    && apt-get install -y \
    adb \
    usbutils \
    nmap \
    && rm -rf /var/lib/apt/lists/*

ENV ADB_DOCKER_WIN="adb -H docker.for.win.localhost"

RUN echo "alias ls='ls --color'" >> /root/.bashrc \
    && echo "echo 0 > /proc/sys/kernel/kptr_restrict" >> /root/.bashrc \
    && echo "source scripts/env.sh" >> /root/.bashrc \
    && echo "export ADB=\"$ADB_DOCKER_WIN\"" >> /root/.bashrc

COPY vimrc /root/.vimrc

CMD ["/bin/bash"]
