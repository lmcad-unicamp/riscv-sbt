all: srcs-get sbt

SRCDIR := $(TOPDIR)/docker/output/src

### get sources ###

srcs-get:
	./get_srcs.py
srcs-clean:
	rm -rf $(SRCDIR)

### riscv-sbt ###

riscv-sbt/src: $(SRCDIR)/riscv-sbt
	rm -rf $@
	cp -a $< $@

.PHONY: riscv-sbt
riscv-sbt: riscv-sbt/src
	docker build -t sbt-$@ $@

### riscv-gnu-toolchain ###

riscv-gnu-toolchain/src: $(SRCDIR)/riscv-gnu-toolchain
	rm -rf $@
	cp -a $< $@

.PHONY: riscv-gnu-toolchain
riscv-gnu-toolchain: riscv-sbt riscv-gnu-toolchain/src
	docker build -t sbt-$@ $@

### llvm ###

llvm/src: $(SRCDIR)/llvm
	rm -rf $@
	cp -a $< $@

.PHONY: llvm
llvm: riscv-gnu-toolchain llvm/src
	docker build -t sbt-$@ $@

### sbt ###

.PHONY: sbt
sbt: llvm
	@echo TODO
	false
	docker build -t $@ $@

###

run:
	docker run -it --rm -h sbt sbt

clean-src:
	rm -rf $(addsuffix /src,riscv-sbt riscv-gnu-toolchain llvm)

clean:
	# remove all container instances
	conts=`docker ps -a | awk '{print$$1}' | sed 1d`; \
	if test -n "$$conts"; then docker rm $$conts; fi
	# remove all '<none>' and 'sbt' images
	imgs=`docker images | grep -e sbt -e '<none>' | awk '{print$$3}'`; \
	if test -n "$$imgs"; then docker rmi $$imgs; fi
