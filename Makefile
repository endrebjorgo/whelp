CC = gcc
CFLAGS = -Wall -Wextra -g -std=c99
TARGET = ./bin/whelp 
SRC = src/main.c 

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(TARGET)

