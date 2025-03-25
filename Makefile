CC = g++
CFLAGS = -Wall -O2 -std=c++17 -Iinclude
TARGET = airport_scheduler
SOURCES = src/main.cpp src/InputReader.cpp src/Scheduler.cpp
OBJECTS = $(SOURCES:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) -o $(TARGET) $(OBJECTS)

src/%.o: src/%.cpp
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f src/*.o $(TARGET)

.PHONY: all clean