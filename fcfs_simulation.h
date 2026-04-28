#ifndef FCFS_SIMULATION_H
#define FCFS_SIMULATION_H

#include "restaurant_parser.h"

#include <string>
#include <vector>

class FCFSSimulation {
public:
    explicit FCFSSimulation(std::vector<Table> tables);

    void runSimulation(const std::vector<Group>& arrivals);
    void setSeatingLogPath(const std::string& path);

private:
    void appendSeatingRecord(const Group& group, const Table& table);
    void initializeSeatingLog() const;
    void processSeating(int currentTime);
    void resetState();

    std::vector<Table> tables;
    std::vector<Group> queue;
    int totalSeatsAvailable = 0;
    int totalSeatMinutesUsed = 0;
    int totalSimulationTime = 0;
    std::string seatingLogPath = "fcfs_seating_log.csv";
};

#endif
