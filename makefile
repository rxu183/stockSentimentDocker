CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++20 -Iinclude
LDFLAGS = -Llib -L$(VCPKG_PATH)/installed/x64-linux/lib -laws-cpp-sdk-core -laws-cpp-sdk-secretsmanager
# search this directory 
LDLIBS = -lcurl -loai -lpqxx #this is where 

SRCDIR = src
INCDIR = include
BINDIR = bin

SOURCES = $(wildcard $(SRCDIR)/*.cpp)
OBJECTS = $(SOURCES:$(SRCDIR)/%.cpp=$(BINDIR)/%.o)
EXECUTABLE = $(BINDIR)/app

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(OBJECTS) $(LDFLAGS) -o $@ $(LDLIBS)

$(BINDIR)/%.o: $(SRCDIR)/%.cpp
	mkdir -p $(BINDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(BINDIR)/*.o $(EXECUTABLE)

run: $(TARGET)
	./bin/$(TARGET)
.PHONY: clean
