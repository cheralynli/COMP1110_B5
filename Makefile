CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -pedantic
TARGET := restaurant_sim
SOURCES := main.cpp simulation.cpp fcfs_simulation.cpp size_queue_simulation.cpp restaurant_parser.cpp
OBJECTS := $(SOURCES:.cpp=.o)

FCFS_TARGET := restaurant_fcfs
FCFS_SOURCES := fcfs_main.cpp fcfs_simulation.cpp restaurant_parser.cpp
FCFS_OBJECTS := $(FCFS_SOURCES:.cpp=.o)

SIZE_TARGET := restaurant_size_queue
SIZE_SOURCES := size_main.cpp size_queue_simulation.cpp restaurant_parser.cpp
SIZE_OBJECTS := $(SIZE_SOURCES:.cpp=.o)

.PHONY: all run run_fcfs run_size clean rebuild

all: $(TARGET) $(FCFS_TARGET) $(SIZE_TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $(TARGET)

$(FCFS_TARGET): $(FCFS_OBJECTS)
	$(CXX) $(CXXFLAGS) $(FCFS_OBJECTS) -o $(FCFS_TARGET)

$(SIZE_TARGET): $(SIZE_OBJECTS)
	$(CXX) $(CXXFLAGS) $(SIZE_OBJECTS) -o $(SIZE_TARGET)

main.o: main.cpp simulation.h fcfs_simulation.h size_queue_simulation.h restaurant_parser.h
	$(CXX) $(CXXFLAGS) -c main.cpp

simulation.o: simulation.cpp simulation.h restaurant_parser.h
	$(CXX) $(CXXFLAGS) -c simulation.cpp

fcfs_main.o: fcfs_main.cpp fcfs_simulation.h restaurant_parser.h
	$(CXX) $(CXXFLAGS) -c fcfs_main.cpp

fcfs_simulation.o: fcfs_simulation.cpp fcfs_simulation.h restaurant_parser.h
	$(CXX) $(CXXFLAGS) -c fcfs_simulation.cpp

size_main.o: size_main.cpp size_queue_simulation.h restaurant_parser.h
	$(CXX) $(CXXFLAGS) -c size_main.cpp

size_queue_simulation.o: size_queue_simulation.cpp size_queue_simulation.h restaurant_parser.h
	$(CXX) $(CXXFLAGS) -c size_queue_simulation.cpp

restaurant_parser.o: restaurant_parser.cpp restaurant_parser.h
	$(CXX) $(CXXFLAGS) -c restaurant_parser.cpp

run: $(TARGET)
	./$(TARGET)

run_fcfs: $(FCFS_TARGET)
	./$(FCFS_TARGET)

run_size: $(SIZE_TARGET)
	./$(SIZE_TARGET)

clean:
	rm -f *.o $(TARGET) $(FCFS_TARGET) $(SIZE_TARGET) $(TARGET).exe $(FCFS_TARGET).exe $(SIZE_TARGET).exe

rebuild: clean all
