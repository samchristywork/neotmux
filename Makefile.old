CC=gcc
CFLAGS=-Isrc/ -g
LIBS=-lvterm

all: build/ntmux

.PHONY: objects
objects: $(patsubst src/%.c, build/%.o, $(wildcard src/*.c))

build/%.o: src/%.c
	mkdir -p build
	$(CC) -c $(CFLAGS) $< -o $@

build/ntmux: objects
	${CC} build/*.o ${LIBS} -o $@

.PHONY: server
server: build/ntmux
	./build/ntmux server

.PHONY: client
client: build/ntmux
	./build/ntmux client

.PHONY: debug
debug: clean build/ntmux
	gdb ./build/ntmux server

.PHONY: watch
watch:
	ls src/*.c | entr make

.PHONY: flamegraph
flamegraph: build/ntmux
	#https://github.com/brendangregg/FlameGraph
	sudo perf record -F 99 -a -g -- ./build/ntmux
	sudo perf script | FlameGraph/stackcollapse-perf.pl | FlameGraph/flamegraph.pl > perf.svg
	firefox perf.svg

.PHONY: clean
clean:
	rm -rf build
