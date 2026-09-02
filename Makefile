CC = gcc

CPPFLAGS = -Iinclude
CFLAGS = -Wall -Wextra -std=c11 -march=native

MODE ?= release

ifeq ($(MODE),debug)
    CFLAGS += -g -O0
else
    CFLAGS += -O3 -DNDEBUG
endif

SRC = $(wildcard src/*.c)
OBJ = $(SRC:.c=.o)
DEP = $(OBJ:.o=.d)

.PHONY: all clean rebuild asm example

all: example

example: $(OBJ) examples/example.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(OBJ) examples/example.c -o $@

src/%.o: src/%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

-include $(DEP)

asm:
	$(CC) $(CPPFLAGS) $(CFLAGS) -S src/$(FILE).c -o $(FILE).s

clean:
	rm -f $(OBJ) $(DEP) example *.s

rebuild: clean all