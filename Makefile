CC=gcc
CFLAGS=-I./src -Wall -g
SRC=$(wildcard src/*.c)
OBJ=$(patsubst src/%.c, build/debug/clox/%.o, $(SRC))
TARGET=orus
RELEASE_TARGET=build/release/clox

debug: $(OBJ)
	@mkdir -p $(dir $(RELEASE_TARGET))
	$(CC) -o $(RELEASE_TARGET) $^
	cp $(RELEASE_TARGET) $(TARGET)

orus: debug

# Rule to build the final binary
$(RELEASE_TARGET): $(OBJ)
	@mkdir -p $(dir $@)
	$(CC) -o $@ $^

# Rule to compile .c files into .o files in debug directory
build/debug/clox/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean rule to remove all generated files
clean:
	rm -rf build $(TARGET) 
