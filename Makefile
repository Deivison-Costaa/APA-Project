# Compilador e flags
CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -I./Include -O3 -fopenmp

# Nome do executável final
TARGET = apa_project

# Fontes
SRCS = src/main.cpp \
       src/Instance.cpp \
       src/GreedyAlgorithm.cpp \
	   src/VariableNeighborhoodDescent.cpp \
	   src/MetaHeuristics.cpp \

# Objetos (substitui .cpp por .o)
OBJS = $(SRCS:.cpp=.o)

# Regra principal
all: $(TARGET)

# Linkagem final + limpeza automática dos .o
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^
	rm -f $(OBJS)


# Compilação dos arquivos fonte para objeto
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Limpeza
clean:
	rm -f $(OBJS) $(TARGET)
