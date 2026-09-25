# Compilador e flags
CXX      = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -I./Include -O3 -fopenmp -MMD -MP
LDFLAGS  = -fopenmp

# Nome do executável final
TARGET = apa_project

SRCS = src/main.cpp \
       src/Instance.cpp \
       src/GreedyAlgorithm.cpp \
       src/VariableNeighborhoodDescent.cpp \
       src/MetaHeuristics.cpp

OBJS = $(SRCS:.cpp=.o)
DEPS = $(OBJS:.o=.d)

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Confere o custo do exemplo do enunciado (deve ser 2800)
test: $(TARGET)
	./$(TARGET) tests/exemplo.txt --check tests/exemplo_solucao.txt

clean:
	rm -f $(OBJS) $(DEPS) $(TARGET)

-include $(DEPS)
