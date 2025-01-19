# Makefile for mouse recorder

CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c99 -O2 -lX11 -lXtst -lasound -lm -lXrandr

LDFLAGS = -lX11
TARGET = wavkit

all: $(TARGET)

$(TARGET): main.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

clean:
	rm -f $(TARGET) *.o

.PHONY: all clean
