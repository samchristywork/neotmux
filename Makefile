all: build/ntmux

.PHONY: objects
objects: $(patsubst src/%.c, build/%.o, $(wildcard src/*.c))

build/ntmux: src/*.c src/*.h objects
	mkdir -p build
	gcc build/*.o -o build/ntmux -lreadline -lvterm -llua5.4

build/%.o: src/%.c src/*.h
	mkdir -p build
	gcc -c $< -o $@ -lreadline -lvterm -llua5.4

simple:
	mkdir -p build
	gcc src/*.c -o build/ntmux -lreadline -lvterm -llua5.4

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

callgraph: build/ntmux
	./scripts/callgraph

clean:
	rm -rf build
