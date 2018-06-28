FROM debian:stretch

# python3: needed by build system
# git: nice to have to pull fixes without rebuilding every image
RUN apt-get update \
    && apt-get install -y \
    git \
    python3 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /riscv-sbt
CMD ["/bin/bash"]
