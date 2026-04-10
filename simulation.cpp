// simulation.cpp
// Implementation file for the restaurant simulation logic.
//
// Planned responsibilities:
// - define all methods declared in simulation.h
// - parse restaurant configuration files
// - parse customer arrival schedule files
// - maintain waiting queues and seated groups over time
// - assign arriving groups to the correct queue based on group size
// - choose the earliest waiting group that fits an available table
// - run the main time advancement loop for the full service period
// - update departures and free tables when dining durations end
// - compute metrics such as average wait, max wait, utilization, and service level
// - optionally write CSV output for downstream Python analysis
#include <iostream>
#include <parser.h>
#include <vector>
#include <list>
#include <map>
#include <cmath>
#include <algorithm>
#include <iomanip>

class WokThisWaySim {
private:
    std::vector<Table> tables;
    std::vector<Group> queue;
    
    double fairnessWeight;    
    int lookAheadWindow;      
    std::map<int, double> arrivalRates;

    int totalSeatsAvailable = 0;
    int totalSeatMinutesUsed = 0;
    int totalSimulationTime = 0;

public:
    WokThisWaySim(std::vector<Table> t, double alpha, int window) 
        : tables(t), fairnessWeight(alpha), lookAheadWindow(window) {
        
        for (const auto& table : tables) {
            totalSeatsAvailable += table.capacity;
        }
    }

    void setArrivalRates(std::map<int, double> rates) {
        arrivalRates = rates;
    }
    double calculateOpportunityCost(int tableCapacity) {
        double expectedValue = 0.0;
        for (int size = 1; size <= tableCapacity; ++size) {
            if (arrivalRates.find(size) == arrivalRates.end()) continue; 
            double lambda = arrivalRates[size];
            double probArrival = 1.0 - std::exp(-lambda * lookAheadWindow);  
            expectedValue += probArrival * (size * 45.0); 
        }
        return expectedValue;
    }

    void processSeating(int currentTime) {
        bool seatedSomeone = true;

        while (seatedSomeone) {
            seatedSomeone = false;

            for (auto& table : tables) {
                if (!table.isFree) continue;

                int bestIdx = -1;
                double bestUtility = -1.0;

                for (int i = 0; i < queue.size(); ++i) {
                    if (queue[i].size <= table.capacity) {
                        int currentWaitTime = currentTime - queue[i].arrivalTime;
                        
                        double utility = (queue[i].size * queue[i].diningDuration) + 
                                         (currentWaitTime * fairnessWeight);

                        if (utility > bestUtility) {
                            bestUtility = utility;
                            bestIdx = i;
                        }
                    }
                }

                if (bestIdx != -1) {
                    Group bestGroup = queue[bestIdx];
                    int currentWaitTime = currentTime - bestGroup.arrivalTime;
                    
                    double expectedValueWait = calculateOpportunityCost(table.capacity);

                    if (bestUtility > expectedValueWait || currentWaitTime >= bestGroup.maxWaitTolerance) {
                        
                        table.isFree = false;
                        table.availableAt = currentTime + bestGroup.diningDuration;
                        bestGroup.seatingTime = currentTime;
                        
                        totalSeatMinutesUsed += (bestGroup.size * bestGroup.diningDuration);
                        
                        std::cout << "Time " << std::setw(3) << currentTime 
                                  << " | Seated Group " << std::setw(2) << bestGroup.id 
                                  << " (Size " << bestGroup.size 
                                  << ") at Table " << table.id << " (Cap " << table.capacity 
                                  << "). Wait time: " << currentWaitTime << " mins.\n";

                        queue.erase(queue.begin() + bestIdx);
                        seatedSomeone = true;                        
                        break; 
                    }
                }
            }

        }
    }

    void runSimulation(std::vector<Group> arrivals) {
        int nextArrivalIdx = 0;
        int currentTime = 0;
        int totalGroups = arrivals.size();

        while (nextArrivalIdx < totalGroups || !queue.empty()) {
            
            int nextEventTime = 999999;
            if (nextArrivalIdx < totalGroups) {
                nextEventTime = arrivals[nextArrivalIdx].arrivalTime;
            }
            for (const auto& table : tables) {
                if (!table.isFree && table.availableAt < nextEventTime) {
                    nextEventTime = table.availableAt;
                }
            }

            currentTime = nextEventTime;
            totalSimulationTime = currentTime;

            for (auto& table : tables) {
                if (!table.isFree && table.availableAt <= currentTime) {
                    table.isFree = true;
                }
            }

            while (nextArrivalIdx < totalGroups && arrivals[nextArrivalIdx].arrivalTime <= currentTime) {
                queue.push_back(arrivals[nextArrivalIdx]);
                nextArrivalIdx++;
            }

            processSeating(currentTime);
        }
        
        printMetrics();
    }

    void printMetrics() {
        std::cout << "\n--- SIMULATION COMPLETE ---\n";
        double maxPossibleSeatMinutes = totalSeatsAvailable * totalSimulationTime;
        double utilization = (totalSeatMinutesUsed / maxPossibleSeatMinutes) * 100.0;
        
        std::cout << "Total Simulation Time: " << totalSimulationTime << " minutes\n";
        std::cout << "Overall Seat Utilization: " << std::fixed << std::setprecision(2) << utilization << "%\n";
    }
};


int main() {
    int table_sizes;
    std::cin >> table_sizes; 
    std::vector<Table> tables;
    for(int i = 1; i <= table_sizes; ++i) {
        int siz;
        std::cin >> siz;
        tables.push_back({i, siz});
    } 
    double alphaWeight = 1.5; 
    int evLookAheadMinutes = 15;
    WokThisWaySim sim(tables, alphaWeight, evLookAheadMinutes);

    std::map<int, double> rates = {
        {1, 0.10},  
        {2, 0.20},  
        {4, 0.05},
        {8, 0.016} 
    };
    sim.setArrivalRates(rates);
    int fixed_wait_tolerance;
    // in case we want to fix the number of tolerance
    std::vector<Group> arrivals = {
        {1, 1, 0,  30, 20},
        {2, 8, 5,  60, 40}, 
        {3, 4, 10, 45, 30}, 
        {4, 2, 12, 35, 20},
        {5, 1, 15, 20, 15},
        {6, 4, 18, 40, 25}
    };

    sim.runSimulation(arrivals);

    return 0;
}
