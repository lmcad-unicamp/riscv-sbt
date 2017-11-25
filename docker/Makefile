all: build

copy_local:
	cp -a ../make .

build:
	docker build -t sbt .

run:
	docker run -it --rm -h sbt sbt

clean:
	# remove all container instances
	docker rm $(docker ps -a | awk '{print$1}' | sed 1d)
	# remove all '<none>' and 'sbt' images
	docker rmi $(docker images | grep -e sbt -e '<none>' | awk '{print$3}')
