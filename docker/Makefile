all: build

copy_local:
	cp -a ../make .

build:
	docker build -t sbt .

run:
	docker run -it --rm -h sbt sbt

clean:
	# remove all container instances
	conts=`docker ps -a | awk '{print$$1}' | sed 1d`; \
	if test -n "$$conts"; then docker rm $$conts; fi
	# remove all '<none>' and 'sbt' images
	imgs=`docker images | grep -e sbt -e '<none>' | awk '{print$$3}'`; \
	if test -n "$$imgs"; then docker rmi $$imgs; fi
