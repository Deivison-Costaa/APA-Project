CXX      ?= g++
CXXFLAGS ?= -O3 -march=native -std=c++20 -Wall -Wextra -pthread
SRC      := $(wildcard src/*.cpp)
OBJ      := $(SRC:src/%.cpp=build/%.o)

hgs: $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

build/%.o: src/%.cpp $(wildcard src/*.hpp) | build
	$(CXX) $(CXXFLAGS) -c -o $@ $<

build:
	mkdir -p build

test: hgs
	./tests/run_tests.sh

clean:
	rm -rf build hgs

.PHONY: test clean
