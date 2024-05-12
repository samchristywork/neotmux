CC=gcc
CFLAGS=-g -Wall -Wpedantic -Wextra -Werror
LIBS=-lreadline -lvterm -llua5.4

all: build/ntmux

.PHONY: objects
objects: $(patsubst src/%.c, build/%.o, $(wildcard src/*.c))

build/ntmux: src/*.c src/*.h objects
	@mkdir -p build
	${CC} ${CFLAGS} build/*.o -o build/ntmux ${LIBS}

build/%.o: src/%.c src/*.h
	@mkdir -p build
	${CC} ${CFLAGS} -c $< -o $@ ${LIBS}

simple:
	@mkdir -p build
	${CC} ${CFLAGS} src/*.c -o build/ntmux ${LIBS}

run: build/ntmux
	unset NTMUX && alacritty -e build/ntmux -n test

distributed: build/ntmux
	unset NTMUX && build/ntmux -n distributed -s &
	unset NTMUX && alacritty -e build/ntmux -n distributed -c

dev: build/ntmux
	alacritty -e build/ntmux -n dev -l dev.log

server: build/ntmux
	unset NTMUX && build/ntmux -n distributed -s

client: build/ntmux
	unset NTMUX && alacritty -e build/ntmux -n distributed -c

valgrind: build/ntmux
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose --log-file=valgrind.log build/ntmux

callgraph: build/ntmux
	./scripts/callgraph

clean:
	rm -rf build
