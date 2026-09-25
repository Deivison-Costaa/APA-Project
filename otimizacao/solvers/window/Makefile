CXX ?= g++
HIGHS_DIR ?= third_party/highs-install
CXXFLAGS ?= -O3 -march=native -std=c++20 -Wall -Wextra -pthread
INCS := -I$(HIGHS_DIR)/include/highs
LIBS := $(HIGHS_DIR)/lib64/libhighs.a -lz -pthread
SRC := $(wildcard src/*.cpp)
OBJ := $(SRC:src/%.cpp=build/%.o)

all: window

window: $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ) $(LIBS)

build/%.o: src/%.cpp $(wildcard src/*.hpp)
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(INCS) -c -o $@ $<

# HiGHS (static) is built once from third_party/HiGHS (v1.15.1):
highs:
	git clone --depth 1 --branch v1.15.1 https://github.com/ERGO-Code/HiGHS.git third_party/HiGHS || true
	mkdir -p third_party/highs-build && cd third_party/highs-build && \
	cmake ../HiGHS -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF -DFAST_BUILD=ON -DBUILD_TESTING=OFF \
	  -DCMAKE_INSTALL_PREFIX=$(abspath $(HIGHS_DIR)) && make -j3 && make install

test: window
	./window ../../instances/exemplo.txt --mode selftest
	./window ../../instances/n500m10E.txt --mode selftest

clean:
	rm -rf build window

.PHONY: all clean test highs
