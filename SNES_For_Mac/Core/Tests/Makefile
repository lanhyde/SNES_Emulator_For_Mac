# Makefile for SNES Emulator CPU Tests

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g
TARGET = test_cpu
SOURCES = test_cpu.cpp ../CPU/CPU65c816.cpp ../Memory/Memory.cpp
HEADERS = ../CPU/CPU65c816.hpp ../Memory/Memory.hpp ../Types/Types.hpp

# Build the test executable
$(TARGET): $(SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET)

# Run the tests
test: $(TARGET)
	./$(TARGET)

# Clean build artifacts
clean:
	rm -f $(TARGET)

.PHONY: test clean
