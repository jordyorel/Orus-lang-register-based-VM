CC=gcc
all: debug
CFLAGS=-I./include -Wall -g
SRC=$(shell find src -name '*.c')
STDLIBC=src/vm/builtin_stdlib.c
STDLIBH=include/builtin_stdlib.h
$(STDLIBC) $(STDLIBH): tools/gen_stdlib.py $(wildcard std/*.orus)
	python3 tools/gen_stdlib.py std $(STDLIBC) $(STDLIBH)
OBJ=$(patsubst src/%.c, build/debug/clox/%.o, $(SRC))
TARGET=orusc
RELEASE_TARGET=build/release/clox

debug: $(OBJ)
	@mkdir -p $(dir $(RELEASE_TARGET))
	$(CC) -o $(RELEASE_TARGET) $^ -lm
	cp $(RELEASE_TARGET) $(TARGET)

orusc: debug

# Rule to build the final binary
$(RELEASE_TARGET): $(OBJ)
	@mkdir -p $(dir $@)
	$(CC) -o $@ $^ -lm

# Rule to compile .c files into .o files in debug directory
build/debug/clox/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean rule to remove all generated files
clean:
	rm -rf build $(TARGET) 
