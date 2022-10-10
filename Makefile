all: build run
build:
	bash build.sh
clean:
	bash clean.sh
run: build
	bash run.sh
