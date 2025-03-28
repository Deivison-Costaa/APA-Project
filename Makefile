# Compilador e flags
CXX = g++
<<<<<<< HEAD
<<<<<<< HEAD
CXXFLAGS = -Wall -Wextra -std=c++17 -I./Include -O2
=======
CXXFLAGS = -Wall -Wextra -std=c++11 -I./Include
>>>>>>> a13f42e (Refatoração da classe instância, agora ela é responsável pela solução)
=======
CXXFLAGS = -Wall -Wextra -std=c++17 -I./Include -O2
>>>>>>> 8d2dbf4 (Criação de duas classes do VND, necessário estudar como unir o valor de uma com a velocidade de execução da outra)

# Nome do executável final
TARGET = apa_project

# Fontes
SRCS = src/main.cpp \
       src/Instance.cpp \
<<<<<<< HEAD
<<<<<<< HEAD
       src/GreedyAlgorithm.cpp \
	   src/VariableNeighborhoodDescent.cpp \
	   src/VND.cpp \
=======
       src/GreedyAlgorithm.cpp
>>>>>>> a13f42e (Refatoração da classe instância, agora ela é responsável pela solução)
=======
       src/GreedyAlgorithm.cpp \
	   src/VariableNeighborhoodDescent.cpp \
	   src/VND.cpp \
>>>>>>> 8d2dbf4 (Criação de duas classes do VND, necessário estudar como unir o valor de uma com a velocidade de execução da outra)

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
