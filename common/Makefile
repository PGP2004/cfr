CXX      := $(shell ls /opt/homebrew/bin/g++-* 2>/dev/null | sort -V | tail -n1)
ifeq ($(CXX),)
CXX    := g++
endif
CXXFLAGS := -std=c++20 -Wall -Wextra -O2 -fopenmp -Iinclude
CXXFLAGS += -I../external/hand-isomorphism/src
CXXFLAGS += -MMD -MP

SRC_DIR   := src
OBJ_DIR   := build/obj
LIB       := libcommon.a

SRCS := $(wildcard $(SRC_DIR)/*.cpp)
OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRCS))

.PHONY: all clean
all: $(LIB)

$(LIB): $(OBJS)
	ar rcs $@ $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $@

-include $(OBJS:.o=.d)

clean:
	rm -rf build $(LIB)