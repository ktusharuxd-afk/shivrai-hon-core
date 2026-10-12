.PHONY: all build test run demo clean docker bench

all: build

build:
	bash scripts/build.sh

test:
	cd build && ctest --output-on-failure

run:
	bash scripts/run_node.sh

demo:
	bash scripts/demo.sh

bench: build
	./build/shivrai-bench

clean:
	rm -rf build data

docker:
	docker compose up --build
