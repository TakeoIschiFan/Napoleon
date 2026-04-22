CC = gcc
CFLAGS = -Wall -Wextra -g
TARGET = napoleon
SRCS = main.c test_example.c
NAPOLEON = ../napoleon.h
README_TARGET = readme_test
README_SRCS = readme_main.c readme_tests.c

all: $(TARGET)

$(TARGET): $(SRCS) $(NAPOLEON)
	$(CC) $(CFLAGS) -o $@ $(SRCS)

test: $(TARGET)
	./$(TARGET)

readme: $(README_TARGET)
	./$(README_TARGET)

$(README_TARGET): $(README_SRCS) $(NAPOLEON)
	$(CC) $(CFLAGS) -o $@ $(README_SRCS)

clean:
	rm -f $(TARGET) $(README_TARGET)

.PHONY: all test readme clean