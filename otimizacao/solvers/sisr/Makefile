CXX ?= g++
CXXFLAGS ?= -O3 -march=native -std=c++20 -Wall -Wextra -pthread
SRC := $(wildcard src/*.cpp)
HDR := $(wildcard src/*.hpp)

sisr: $(SRC) $(HDR)
	$(CXX) $(CXXFLAGS) -o $@ $(SRC)

sisr_debug: $(SRC) $(HDR)
	$(CXX) -O1 -g -std=c++20 -Wall -Wextra -pthread -D_GLIBCXX_ASSERTIONS -D_GLIBCXX_DEBUG -o $@ $(SRC)

test: sisr sisr_debug
	./tests/selftest.sh

clean:
	rm -f sisr sisr_debug

.PHONY: test clean
