CC = gcc
CFLAGS = -Wall -Wextra -g

# Target executable
TARGET = ./bin/whelp 

# Source files
SRC = src/main.c src/lp.c

# Default target
all: $(TARGET)

# Link object files to create the executable
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^

# Clean up generated files
clean:
	rm -f $(TARGET)

# Phony targets (not files)
.PHONY: all clean

