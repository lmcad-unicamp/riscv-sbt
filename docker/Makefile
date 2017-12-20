all: srcs-get sbt-builder

SRCDIR := $(TOPDIR)/docker/output/src

riscv-sbt/src: $(SRCDIR)/riscv-sbt
	rm -rf $@
	cp -a $< $@

.PHONY: riscv-sbt
riscv-sbt: riscv-sbt/src
	docker build -t sbt-$@ $@

riscv-gnu-toolchain/src: $(SRCDIR)/riscv-gnu-toolchain
	rm -rf $@
	cp -a $< $@

.PHONY: riscv-gnu-toolchain
riscv-gnu-toolchain: riscv-gnu-toolchain/src
	docker build -t sbt-$@ $@

srcs-get:
	./get_srcs.py
srcs-clean:
	rm -rf $(SRCDIR)

sbt-builder:
	docker build -t sbt-builder builder

sbt-data:
	docker create -v --name sbt-data

BUILDER := docker run -it -v $(PWD)/riscv-sbt:/riscv-sbt sbt-builder
toolchain:
	$(BUILDER) llvm
	$(BUILDER) riscv-gnu-toolchain-linux
	$(BUILDER) spike
	$(BUILDER) qemu
	cp -a riscv-sbt/toolchain .

sbt-toolchain:
	docker build -t sbt-toolchain -f Dockerfile.toolchain .
	rm -rf toolchain

sbt:
	docker build -t sbt .
sbt-save:
	docker save sbt | gzip -c > sbt.tgz

run:
	docker run -it --rm -h sbt sbt

clean:
	# remove all container instances
	conts=`docker ps -a | awk '{print$$1}' | sed 1d`; \
	if test -n "$$conts"; then docker rm $$conts; fi
	# remove all '<none>' and 'sbt' images
	imgs=`docker images | grep -e sbt -e '<none>' | awk '{print$$3}'`; \
	if test -n "$$imgs"; then docker rmi $$imgs; fi
