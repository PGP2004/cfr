CXX := g++
OMP := /opt/homebrew/opt/libomp

CXXFLAGS := -std=c++20 -Wall -Wextra -g -O3 -march=native -funroll-loops -fno-math-errno \
    -Xpreprocessor -fopenmp -I$(OMP)/include \
    -Icommon/include -Isolver/include -Ibot_arena/include -Iclustering/include \
    -Iexternal/tomlplusplus -Iexternal/hand-isomorphism/src

LDFLAGS := -Lexternal/hand-isomorphism -lhand_index \
    -L$(OMP)/lib -lomp -Wl,-rpath,$(OMP)/lib

OBJDIR := build/obj
obj = $(patsubst %.cpp,$(OBJDIR)/%.o,$(1))

COMMON_SRCS := $(wildcard common/src/*.cpp)
SOLVER_SRCS := $(filter-out solver/src/main.cpp,$(wildcard solver/src/*.cpp))
ARENA_SRCS  := $(wildcard bot_arena/src/*.cpp)
CLUST_SRCS  := $(wildcard clustering/src/*.cpp)

COMMON_OBJS := $(call obj,$(COMMON_SRCS))
LIB_OBJS    := $(COMMON_OBJS) $(call obj,$(SOLVER_SRCS))
TRAIN_OBJS  := $(LIB_OBJS) $(call obj,solver/src/main.cpp)
ARENA_OBJS  := $(LIB_OBJS) $(call obj,$(ARENA_SRCS))
CLUST_OBJS  := $(COMMON_OBJS) $(call obj,$(CLUST_SRCS))

DEPS := $(sort $(TRAIN_OBJS) $(ARENA_OBJS) $(CLUST_OBJS))
DEPS := $(DEPS:.o=.d)

all: train arena clustering

train: build/train
arena: build/arena
clustering: build/clustering

build/train: $(TRAIN_OBJS)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

build/arena: $(ARENA_OBJS)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

build/clustering: $(CLUST_OBJS)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

clean:
	rm -rf build

-include $(DEPS)

.PHONY: all train arena clustering clean