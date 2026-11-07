CC ?= cc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Werror
all: build/ork-test build/ork-render build/adapter-test
build:
	mkdir -p build
build/ork-test: src/ork.c src/ork.h tests/test_core.c | build
	$(CC) $(CFLAGS) -Isrc src/ork.c tests/test_core.c -o $@
build/ork-render: src/ork.c src/ork.h tools/render.c | build
	$(CC) $(CFLAGS) -Isrc src/ork.c tools/render.c -o $@
build/adapter-test: tests/test_adapter.c device/paleotone_adapter.c src/ork.c src/ork.h | build
	mkdir -p build/ork
	cp src/ork.c src/ork.h build/ork/
	$(CC) $(CFLAGS) -Ibuild -Idevice tests/test_adapter.c -o $@
test: all
	python3 tests/make_fixture.py build/fixture.bin
	build/ork-test build/fixture.bin
	build/adapter-test
.PHONY: all test
