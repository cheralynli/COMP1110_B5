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

#include "restaurant_parser.h"

#include <map>
#include <string>
#include <vector>

enum class AlgorithmType {
    Custom,
    FCFS,
    SizeQueue,
};

struct SimulationSummary {
    int groupsServed = 0;
    double averageWait = 0.0;
    int maxWait = 0;
    double tableUtilization = 0.0;
    double serviceLevel15 = 0.0;
    int maxQueueLength = 0;
    int totalSimulationTime = 0;
};

class WokThisWaySim {
public:
    WokThisWaySim(
        std::vector<Table> tables,
        std::vector<QueueRule> queueRules,
        AlgorithmType algorithm,
        double fairnessWeight,
        int lookAheadWindow
    );

    void precomputeHourlyRates(const std::vector<Group>& historicalData);
    SimulationSummary runSimulation(const std::vector<Group>& arrivals);
    void setSeatingLogPath(const std::string& path);

private:
    void appendSeatingRecord(const Group& group, const Table& table);
    double calculateOpportunityCost(int tableCapacity, int currentTime) const;
    void initializeSeatingLog() const;
    void processSeating(int currentTime);
    void resetState();
    int selectNextGroupIndex(const Table& table, int currentTime) const;
    int selectCustomGroupIndex(const Table& table, int currentTime) const;
    int selectFcfsGroupIndex(const Table& table) const;
    int selectSizeQueueGroupIndex(const Table& table) const;

    std::vector<Table> tables;
    std::vector<Group> queue;
    std::vector<QueueRule> queueRules;
    AlgorithmType algorithm;
    double fairnessWeight;
    int lookAheadWindow;
    std::map<int, std::map<int, double>> hourlyArrivalRates;
    int totalSeatsAvailable = 0;
    int totalSeatMinutesUsed = 0;
    int totalSimulationTime = 0;
    int totalWaitTime = 0;
    int maxWaitTime = 0;
    int groupsServed = 0;
    int groupsWithin15 = 0;
    int maxQueueLength = 0;
    std::string seatingLogPath = "seating_log.csv";
};

#endif
