# Makefile for mouse recorder

CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c99 -O2 -o wavkit -lX11 -lXtst -lXrandr -lasound -lm


LDFLAGS = -lX11
TARGET = wavkit

all: $(TARGET)

$(TARGET): main.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

clean:
	rm -f $(TARGET) *.o

.PHONY: all clean
