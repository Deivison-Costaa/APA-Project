# Compilador e flags
CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++11 -I./Include -I./Include/InstanceReader

# Nome do executável
TARGET = apa_project

# Fontes
SRCS = src/main.cpp \
       Include/InstanceReader/InstanceReader.cpp

# Objetos
OBJS = $(SRCS:.cpp=.o)

# Regra padrão
all: $(TARGET)

# Linkagem final
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Compilação dos arquivos .cpp
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Limpeza dos objetos e binários
clean:
	rm -f $(OBJS) $(TARGET)
