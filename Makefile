.PHONY: all clean

# Default target
all: main main2.s

clean:
	git clean -dfX --exclude="!/.vscode"

main: main.c
	clang --debug -std=c11 -pedantic -Wall -Wno-gnu-case-range \
		-fprofile-instr-generate -fcoverage-mapping -mmacosx-version-min=14.7 \
		-o $@ $<

main2.s: main main.c
	./main main.c > main2.s

main2: main2.s
	clang -O0 --debug -o $@ $<
