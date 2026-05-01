#ifndef SIZE_QUEUE_SIMULATION_H
#define SIZE_QUEUE_SIMULATION_H

#include "restaurant_parser.h"

#include <string>
#include <vector>

class SizeQueueSimulation {
public:
    explicit SizeQueueSimulation(std::vector<Table> tables);

    void runSimulation(const std::vector<Group>& arrivals);
    void setSeatingLogPath(const std::string& path);

private:
    void appendSeatingRecord(const Group& group, const Table& table);
    void enqueueGroup(const Group& group);
    bool hasWaitingGroups() const;
    void initializeSeatingLog() const;
    void processSeating(int currentTime);
    void resetState();

    std::vector<Table> tables;
    std::vector<Group> smallQueue;
    std::vector<Group> mediumQueue;
    std::vector<Group> largeQueue;
    int totalSeatsAvailable = 0;
    int totalSeatMinutesUsed = 0;
    int totalSimulationTime = 0;
    std::string seatingLogPath = "size_seating_log.csv";
};

#endif
