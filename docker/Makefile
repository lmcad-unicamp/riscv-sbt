all: build

copy_local:
	cp -a ../make .

build:
	docker build -t sbt .

run:
	docker run -it --rm -h sbt sbt
