# === Makefile for a simple C project ===

# Compiler and flags
CC := gcc
CFLAGS := -Wall -Wextra -std=c11 -O2

# Automatically detect all .c files in the current directory
SRC := $(wildcard *.c)
OBJ := $(SRC:.c=.o)

# Output binary name
TARGET := main

# Default rule
all: $(TARGET)

# Linking rule
$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $@

# Compilation rule
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up build artifacts
clean:
	rm -f $(OBJ) $(TARGET)

# Run the program
run: $(TARGET)
	./$(TARGET)

# Rebuild from scratch
rebuild: clean all

.PHONY: all clean run rebuild
