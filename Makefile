CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -pedantic
TARGET := restaurant_sim
SOURCES := main.cpp simulation.cpp restaurant_parser.cpp
OBJECTS := $(SOURCES:.cpp=.o)

FCFS_TARGET := restaurant_fcfs
FCFS_SOURCES := fcfs_main.cpp fcfs_simulation.cpp restaurant_parser.cpp
FCFS_OBJECTS := $(FCFS_SOURCES:.cpp=.o)

.PHONY: all run run_fcfs clean rebuild

all: $(TARGET) $(FCFS_TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $(TARGET)

$(FCFS_TARGET): $(FCFS_OBJECTS)
	$(CXX) $(CXXFLAGS) $(FCFS_OBJECTS) -o $(FCFS_TARGET)

main.o: main.cpp simulation.h restaurant_parser.h
	$(CXX) $(CXXFLAGS) -c main.cpp

simulation.o: simulation.cpp simulation.h restaurant_parser.h
	$(CXX) $(CXXFLAGS) -c simulation.cpp

fcfs_main.o: fcfs_main.cpp fcfs_simulation.h restaurant_parser.h
	$(CXX) $(CXXFLAGS) -c fcfs_main.cpp

fcfs_simulation.o: fcfs_simulation.cpp fcfs_simulation.h restaurant_parser.h
	$(CXX) $(CXXFLAGS) -c fcfs_simulation.cpp

restaurant_parser.o: restaurant_parser.cpp restaurant_parser.h
	$(CXX) $(CXXFLAGS) -c restaurant_parser.cpp

run: $(TARGET)
	./$(TARGET)

run_fcfs: $(FCFS_TARGET)
	./$(FCFS_TARGET)

clean:
	rm -f *.o $(TARGET) $(FCFS_TARGET) $(TARGET).exe $(FCFS_TARGET).exe

rebuild: clean all
