CC = gcc
CFLAGS = -Wall -Wextra -g
TARGET = ./bin/whelp 
SRC = src/main.c src/lp.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(TARGET)

