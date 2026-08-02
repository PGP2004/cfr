CXX := $(shell ls /opt/homebrew/bin/g++-* 2>/dev/null | sort -V | tail -n1)
ifeq ($(CXX),)
CXX := g++
endif

CXXFLAGS := -std=c++20 -Wall -Wextra -O2 -fopenmp -MMD -MP
CXXFLAGS += -Ik_means/include -Ipipeline/include

BUILD_DIR := build
OBJ_DIR   := $(BUILD_DIR)/obj
BIN_DIR   := $(BUILD_DIR)/bin

ISO_DIR := ../external/hand-isomorphism
ISO_LIB := $(ISO_DIR)/libhand_index.a
CXXFLAGS += -I$(ISO_DIR)/src

COMMON_DIR := ../common
COMMON_LIB := $(COMMON_DIR)/libcommon.a
CXXFLAGS += -I$(COMMON_DIR)/include

TARGET := $(BIN_DIR)/abstraction

SRCS := $(wildcard k_means/src/*.cpp) $(wildcard pipeline/src/*.cpp)
OBJS := $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(SRCS))

.PHONY: all clean common
all: $(TARGET)

$(TARGET): $(OBJS) $(COMMON_LIB) $(ISO_LIB) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $^ -lm -o $@

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(COMMON_LIB): common
common:
	$(MAKE) -C $(COMMON_DIR)

$(BIN_DIR):
	mkdir -p $@

-include $(OBJS:.o=.d)

clean:
	rm -rf $(BUILD_DIR)