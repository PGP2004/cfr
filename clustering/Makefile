CXX        := g++
COMMON_DIR := ../common
ISO_DIR    := ../external/hand-isomorphism
TOML_DIR   := ../external/tomlplusplus
OMP_PREFIX := /opt/homebrew/opt/libomp

OPTFLAGS := -O2
OMPFLAGS := -Xpreprocessor -fopenmp -I$(OMP_PREFIX)/include

CXXFLAGS := -std=c++20 -Wall -Wextra -g $(OPTFLAGS) $(OMPFLAGS) \
    -Iinclude -I$(COMMON_DIR)/include -I$(ISO_DIR)/src -I$(TOML_DIR)

LDFLAGS := -L$(ISO_DIR) -lhand_index \
    -L$(OMP_PREFIX)/lib -lomp -Wl,-rpath,$(OMP_PREFIX)/lib

TARGET   := main
SRCDIR   := src
BUILDDIR := build
OBJDIR   := $(BUILDDIR)/obj
BINDIR   := $(BUILDDIR)/bin

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