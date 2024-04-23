CC=gcc
CFLAGS=-Isrc/ -g
LIBS=

all: build/libtermemu.so

.PHONY: objects
objects: $(patsubst src/%.c, build/%.o, $(wildcard src/*.c))

build/%.o: src/%.c
	mkdir -p build
	$(CC) -c $(CFLAGS) $< -o $@

build/libtermemu.so: src/libtermemu.c
	mkdir -p build
	$(CC) -Isrc/ -shared -fPIC -o $@ src/libtermemu.c $(LIBS)

.PHONY: watch
watch:
	ls src/*.c src/*.h | entr make test

.PHONY: clean
clean:
	rm -rf build
