all: src toolchain sbt

src:
	./get_srcs.sh
src-clean:
	rm -rf riscv-sbt

toolchain:
	docker build -t sbt-toolchain -f Dockerfile.toolchain .
toolchain-save:
	docker save sbt-toolchain | gzip -c > sbt-toolchain.tgz

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
