CXX        := g++
COMMON_DIR := ../common
CXXFLAGS   := -std=c++20 -Wall -Wextra -g -O2 -Iinclude -I$(COMMON_DIR)/include -I../external/hand-isomorphism/src
LDFLAGS    := -L../external/hand-isomorphism -lhand_index
TARGET     := main
SRCDIR     := src
BUILDDIR   := build
OBJDIR     := $(BUILDDIR)/obj
BINDIR     := $(BUILDDIR)/bin

SRCS        := $(wildcard $(SRCDIR)/*.cpp)
OBJS        := $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(SRCS))
COMMON_SRCS := $(wildcard $(COMMON_DIR)/src/*.cpp)
COMMON_OBJS := $(patsubst $(COMMON_DIR)/src/%.cpp,$(OBJDIR)/common_%.o,$(COMMON_SRCS))

$(BINDIR)/$(TARGET): $(OBJS) $(COMMON_OBJS) | $(BINDIR)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS) $(COMMON_OBJS) $(LDFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)/common_%.o: $(COMMON_DIR)/src/%.cpp | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR) $(BINDIR):
	mkdir -p $@

clean:
	rm -rf $(BUILDDIR)

.PHONY: clean