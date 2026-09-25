CXX      ?= g++
HIGHSDIR ?= $(abspath ../../../venv/lib/python3.14/site-packages/highspy)
CXXFLAGS ?= -O3 -march=native -std=c++20 -Wall -Wextra -g
LDFLAGS  += -L$(HIGHSDIR) -l:libhighs.so.1 -Wl,-rpath,$(HIGHSDIR) -pthread

SRC := $(wildcard src/*.cpp)
OBJ := $(SRC:src/%.cpp=build/%.o)
DEP := $(OBJ:.o=.d)

LBSRC := $(wildcard lb/*.cpp)
LBOBJ := $(LBSRC:lb/%.cpp=build/lb_%.o)
DEP   += $(LBOBJ:.o=.d)

all: ilssp winlb

winlb: $(LBOBJ) build/instance.o build/solution.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

build/lb_%.o: lb/%.cpp | build
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

ilssp: $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

build/%.o: src/%.cpp | build
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

# HiGHS-free build: everything except the set-partitioning solver (run with --no-sp).
PUREOBJ := $(filter-out build/spsolver.o,$(OBJ)) build/pure_spsolver_stub.o

ilssp-pure: $(PUREOBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ -pthread

build/pure_%.o: pure/%.cpp | build
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

build:
	mkdir -p build

test: ilssp
	./tests/selftest.sh

clean:
	rm -rf build ilssp winlb

-include $(DEP)

.PHONY: all clean test
