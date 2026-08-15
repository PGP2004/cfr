CXX := g++
COMMON_DIR := ../common
TOML_DIR := ../external/tomlplusplus
OMP_PREFIX := /opt/homebrew/opt/libomp

OPTFLAGS := -O3 -march=native -funroll-loops -fno-math-errno
OMPFLAGS := -Xpreprocessor -fopenmp -I$(OMP_PREFIX)/include

CXXFLAGS := -std=c++20 -Wall -Wextra -g $(OPTFLAGS) $(OMPFLAGS) \
    -Iinclude -I$(COMMON_DIR)/include -I../external/hand-isomorphism/src \
    -I$(TOML_DIR)

LDFLAGS := -L../external/hand-isomorphism -lhand_index \
    -L$(OMP_PREFIX)/lib -lomp -Wl,-rpath,$(OMP_PREFIX)/lib

TARGET := main
SRCDIR := src
BUILDDIR := build
OBJDIR := $(BUILDDIR)/obj
BINDIR := $(BUILDDIR)/bin

SRCS := $(wildcard $(SRCDIR)/*.cpp)
OBJS := $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(SRCS))
COMMON_SRCS := $(wildcard $(COMMON_DIR)/src/*.cpp)
COMMON_OBJS := $(patsubst $(COMMON_DIR)/src/%.cpp,$(OBJDIR)/common_%.o,$(COMMON_SRCS))
DEPS := $(OBJS:.o=.d) $(COMMON_OBJS:.o=.d)

$(BINDIR)/$(TARGET): $(OBJS) $(COMMON_OBJS) | $(BINDIR)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS) $(COMMON_OBJS) $(LDFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

$(OBJDIR)/common_%.o: $(COMMON_DIR)/src/%.cpp | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

$(OBJDIR) $(BINDIR):
	mkdir -p $@

clean:
	rm -rf $(BUILDDIR)

-include $(DEPS)

.PHONY: clean