#
# Makefile
# author: Kevin Waugh (waugh@cs.cmu.edu)
# date: April 7, 2013
#
# see README.md and LICENSE.txt for more details.
#
CFLAGS     ?=-std=c99 -Wall -g -O3
LDLIBS     ?=-lm
SRC        :=$(addprefix src/,deck.c hand_index.c)
OBJ        :=$(SRC:.c=.o)
TARGETS    :=src/check
TARGET_SRC :=$(addprefix src/,$(TARGETS:=-main.c))
TARGET_OBJ :=$(TARGET_SRC:.c=.o)
LIB        :=libhand_index.a

.PHONY: all clean check

all: $(TARGETS) $(LIB)

clean:
	rm -f $(TARGETS)
	rm -f $(MAIN_OBJ) src/check-main.o $(OBJ)
	rm -f $(LIB)

check: src/check
	./src/check

src/check: src/check-main.o $(OBJ)
$(TARGETS):
	$(CC) $(LDFLAGS) -o $@ $^ $(LOADLIBES) $(LDLIBS)

$(LIB): $(OBJ)
	ar rcs $@ $^

src/check-main.o src/hand_index.o: src/deck.h src/hand_index.h
src/deck.o: src/deck.h