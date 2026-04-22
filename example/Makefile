CC = gcc
CFLAGS = -Wall -Wextra -g
NAPOLEON = ../napoleon.h
README_TARGET = readme_test
README_SRCS = readme_main.c readme_tests.c

readme: $(README_TARGET)
	./$(README_TARGET)

$(README_TARGET): $(README_SRCS) $(NAPOLEON)
	$(CC) $(CFLAGS) -o $@ $(README_SRCS)

clean:
	rm -f $(README_TARGET)