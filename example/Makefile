CC = gcc
CFLAGS = -Wall -Wextra -g
TARGET = napoleon
SRCS = main.c test_example.c
NAPOLEON = ../napoleon.h

all: $(TARGET)

$(TARGET): $(SRCS) $(NAPOLEON)
	$(CC) $(CFLAGS) -o $@ $(SRCS)

test: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all test clean