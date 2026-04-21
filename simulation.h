// simulation.h
// Declarations for the restaurant simulation system.
//
// Planned contents:
// - includes for standard library containers and utilities
// - data structures representing:
//   Group: arriving customer party details
//   Table: seating capacity, availability, and identifier
//   WaitingGroup: queue-related metadata for a group waiting to be seated
// - result/metrics structure for simulation output
// - RestaurantSimulation class declaration
// - method signatures for:
//   loading restaurant configuration
//   loading customer arrivals
//   assigning groups to queues
//   selecting the earliest waiting group that fits each table
//   advancing the simulation clock
//   updating seating and departures
//   calculating performance metrics
//   exporting results for analysis

#ifndef SIMULATION_H
#define SIMULATION_H

#include "parser.h"

#include <map>
#include <string>
#include <vector>

class WokThisWaySim {
public:
    WokThisWaySim(std::vector<Table> tables, double fairnessWeight, int lookAheadWindow);

    void precomputeHourlyRates(const std::vector<Group>& historicalData);
    void runSimulation(const std::vector<Group>& arrivals);

private:
    void appendSeatingRecord(const Group& group, const Table& table);
    double calculateOpportunityCost(int tableCapacity, int currentTime);
    void initializeSeatingLog() const;
    void processSeating(int currentTime);
    void resetState();

    std::vector<Table> tables;
    std::vector<Group> queue;
    double fairnessWeight;
    int lookAheadWindow;
    std::map<int, std::map<int, double>> hourlyArrivalRates;
    int totalSeatsAvailable = 0;
    int totalSeatMinutesUsed = 0;
    int totalSimulationTime = 0;
    std::string seatingLogPath = "COMP1110_B5/seating_log.csv";
};

#endif

