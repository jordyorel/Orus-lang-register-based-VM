CC=gcc
all: debug
CFLAGS=-I./include -Wall -g -std=c99 -D_POSIX_C_SOURCE=200809L
SRC=$(shell find src -name '*.c')
STDLIBC=src/vm/builtin_stdlib.c
STDLIBH=include/builtin_stdlib.h

# Generate stdlib only if tools exist
ifneq (,$(wildcard tools/gen_stdlib.py))
$(STDLIBC) $(STDLIBH): tools/gen_stdlib.py $(wildcard std/*.orus)
	python3 tools/gen_stdlib.py std $(STDLIBC) $(STDLIBH)
endif

OBJ=$(patsubst src/%.c, build/debug/clox/%.o, $(SRC))
TARGET=orusc
RELEASE_TARGET=build/release/clox
TEST_TARGET=build/test/test_register_vm

debug: $(OBJ)
	@mkdir -p $(dir $(RELEASE_TARGET))
	$(CC) -o $(RELEASE_TARGET) $^ -lm
	cp $(RELEASE_TARGET) $(TARGET)

orusc: debug

# Test targets
test: $(TEST_TARGET)
	./$(TEST_TARGET)

$(TEST_TARGET): tests/test_register_vm.c $(filter-out build/debug/clox/main.o, $(OBJ))
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ $^ -lm

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

# Development helpers
.PHONY: all debug clean test orusc 
