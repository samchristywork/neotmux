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

.PHONY: clean
clean:
	rm -rf build
