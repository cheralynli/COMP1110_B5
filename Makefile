CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -pedantic
TARGET := restaurant_sim
SOURCES := main.cpp simulation.cpp restaurant_parser.cpp
OBJECTS := $(SOURCES:.cpp=.o)

.PHONY: all run clean rebuild

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $(TARGET)

main.o: main.cpp simulation.h restaurant_parser.h
	$(CXX) $(CXXFLAGS) -c main.cpp -o main.o

simulation.o: simulation.cpp simulation.h restaurant_parser.h
	$(CXX) $(CXXFLAGS) -c simulation.cpp -o simulation.o

restaurant_parser.o: restaurant_parser.cpp restaurant_parser.h
	$(CXX) $(CXXFLAGS) -c restaurant_parser.cpp -o restaurant_parser.o

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJECTS) $(TARGET)

rebuild: clean all
