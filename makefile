# Docker Environment Paths
VCPKG_PATH = /workspace/vcpkg

# Compiler and Flags
CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++20 -Iinclude -I$(VCPKG_PATH)/installed/arm64-linux/include

# Linker Flags and Libraries (Static Libraries)
LDFLAGS = -L$(VCPKG_PATH)/installed/arm64-linux/lib -L/workspace/lib

# In summary, there are a few things that are important - you need to provide the direction to which the compiled library is located, and ... that's about it .
LDLIBS = -loai -lpqxx -lcurl -lssl -lcrypto -lz -lpthread -ldl

# Directories
SRCDIR = src
INCDIR = include
BINDIR = bin

# Source and Object Files
SOURCES = $(wildcard $(SRCDIR)/*.cpp)
OBJECTS = $(SOURCES:$(SRCDIR)/%.cpp=$(BINDIR)/%.o)
EXECUTABLE = $(BINDIR)/app

# Default Target
all: $(EXECUTABLE)

# Link Objects to Create Executable
$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(OBJECTS) $(LDFLAGS) -o $@ $(LDLIBS)

# Compile Source Files into Object Files
$(BINDIR)/%.o: $(SRCDIR)/%.cpp
	mkdir -p $(BINDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean Up
clean:
	rm -f $(BINDIR)/*.o $(EXECUTABLE)

# Run the Executable
run: $(EXECUTABLE)
	./$(EXECUTABLE)

.PHONY: clean run