#include "simulation.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>

namespace {

bool compareGroupsByArrival(const Group& left, const Group& right) {
    if (left.arrivalTime != right.arrivalTime) {
        return left.arrivalTime < right.arrivalTime;
    }

    return left.id < right.id;
}

bool hasOccupiedTables(const std::vector<Table>& tables) {
    for (const Table& table : tables) {
        if (!table.isFree) {
            return true;
        }
    }

    return false;
}

bool hasTableThatFits(const std::vector<Table>& tables, int groupSize) {
    for (const Table& table : tables) {
        if (table.capacity >= groupSize) {
            return true;
        }
    }

    return false;
}

int findNextQueueDeadline(
    const std::vector<Group>& queue,
    const std::vector<Table>& tables,
    int currentTime
) {
    int nextDeadline = std::numeric_limits<int>::max();

    for (const Group& group : queue) {
        bool canUseFreeTable = false;
        for (const Table& table : tables) {
            if (table.isFree && table.capacity >= group.size) {
                canUseFreeTable = true;
                break;
            }
        }

        if (!canUseFreeTable) {
            continue;
        }

        const int deadline = group.arrivalTime + group.maxWaitTolerance;
        if (deadline > currentTime && deadline < nextDeadline) {
            nextDeadline = deadline;
        }
    }

    return nextDeadline;
}

} // namespace

WokThisWaySim::WokThisWaySim(std::vector<Table> inputTables, double alpha, int window) {
    tables = inputTables;
    fairnessWeight = alpha;
    lookAheadWindow = window;

    for (const Table& table : tables) {
        totalSeatsAvailable += table.capacity;
    }
}

void WokThisWaySim::setSeatingLogPath(const std::string& path) {
    seatingLogPath = path;
}

void WokThisWaySim::resetState() {
    queue.clear();
    totalSeatMinutesUsed = 0;
    totalSimulationTime = 0;

    for (auto& table : tables) {
        table.isFree = true;
        table.availableAt = 0;
    }
}

void WokThisWaySim::initializeSeatingLog() const {
    std::ofstream output(seatingLogPath);
    if (!output.is_open()) {
        return;
    }

    output << "group_id,group_size,arrival_time,seating_time,wait_time,table_id,"
           << "table_capacity,dining_duration,departure_time\n";
}

void WokThisWaySim::appendSeatingRecord(const Group& group, const Table& table) {
    std::ofstream output(seatingLogPath, std::ios::app);
    if (!output.is_open()) {
        return;
    }

    const int wait_time = group.seatingTime - group.arrivalTime;
    const int departure_time = group.seatingTime + group.diningDuration;

    output << group.id << ','
           << group.size << ','
           << group.arrivalTime << ','
           << group.seatingTime << ','
           << wait_time << ','
           << table.id << ','
           << table.capacity << ','
           << group.diningDuration << ','
           << departure_time << '\n';
}

void WokThisWaySim::precomputeHourlyRates(const std::vector<Group>& historicalData) {
    hourlyArrivalRates.clear();

    std::map<int, std::map<int, int>> hourlyCounts;
    for (const Group& group : historicalData) {
        const int arrival_hour = group.arrivalTime / 60;
        hourlyCounts[arrival_hour][group.size]++;
    }

    for (const auto& hour_pair : hourlyCounts) {
        const int hour = hour_pair.first;
        for (const auto& size_pair : hour_pair.second) {
            const int size = size_pair.first;
            const int count = size_pair.second;
            const double lambda = static_cast<double>(count) / 60.0;
            hourlyArrivalRates[hour][size] = lambda;
        }
    }
}

double WokThisWaySim::calculateOpportunityCost(int tableCapacity, int currentTime) {
    if (hourlyArrivalRates.empty()) {
        return 0.0;
    }

    double expectedValue = 0.0;
    int currentHour = currentTime / 60;

    if (hourlyArrivalRates.find(currentHour) == hourlyArrivalRates.end()) {
        currentHour = hourlyArrivalRates.rbegin()->first;
    }

    const std::map<int, double>& currentRates = hourlyArrivalRates.at(currentHour);
    for (int size = 1; size <= tableCapacity; ++size) {
        if (currentRates.find(size) == currentRates.end()) {
            continue;
        }

        const double lambda = currentRates.at(size);
        const double probArrival = 1.0 - std::exp(-lambda * lookAheadWindow);
        expectedValue += probArrival * (size * 45.0);
    }

    return expectedValue;
}

void WokThisWaySim::processSeating(int currentTime) {
    bool seatedSomeone = true;
    while (seatedSomeone) {
        seatedSomeone = false;

        for (Table& table : tables) {
            if (!table.isFree) {
                continue;
            }

            int bestIdx = -1;
            double bestUtility = -1.0;
            const int queue_size = queue.size();

            for (int i = 0; i < queue_size; ++i) {
                if (queue[i].size > table.capacity) {
                    continue;
                }

                const int current_wait_time = currentTime - queue[i].arrivalTime;
                const double utility =
                    (queue[i].size * queue[i].diningDuration) + (current_wait_time * fairnessWeight);

                if (utility > bestUtility) {
                    bestUtility = utility;
                    bestIdx = i;
                }
            }

            if (bestIdx == -1) {
                continue;
            }

            Group bestGroup = queue[bestIdx];
            const int current_wait_time = currentTime - bestGroup.arrivalTime;
            const double expected_value_wait = calculateOpportunityCost(table.capacity, currentTime);

            if (bestUtility <= expected_value_wait &&
                current_wait_time < bestGroup.maxWaitTolerance) {
                continue;
            }

            table.isFree = false;
            table.availableAt = currentTime + bestGroup.diningDuration;
            bestGroup.seatingTime = currentTime;
            totalSeatMinutesUsed += bestGroup.size * bestGroup.diningDuration;
            appendSeatingRecord(bestGroup, table);

            std::cout << "Time " << std::setw(3) << currentTime
                      << " | Group " << std::setw(2) << bestGroup.id
                      << " (Size " << bestGroup.size << ") @ Table " << table.id << "\n";

            queue.erase(queue.begin() + bestIdx);
            seatedSomeone = true;
            break;
        }
    }
}

void WokThisWaySim::runSimulation(const std::vector<Group>& arrivals) {
    resetState();
    initializeSeatingLog();

    std::vector<Group> orderedArrivals = arrivals;
    std::sort(orderedArrivals.begin(), orderedArrivals.end(), compareGroupsByArrival);

    int nextArrivalIdx = 0;
    int currentTime = 0;
    const int totalGroups = orderedArrivals.size();

    while (nextArrivalIdx < totalGroups || !queue.empty() || hasOccupiedTables(tables)) {
        int nextEventTime = std::numeric_limits<int>::max();

        if (nextArrivalIdx < totalGroups) {
            nextEventTime = orderedArrivals[nextArrivalIdx].arrivalTime;
        }

        for (const Table& table : tables) {
            if (!table.isFree && table.availableAt < nextEventTime) {
                nextEventTime = table.availableAt;
            }
        }

        const int nextQueueDeadline = findNextQueueDeadline(queue, tables, currentTime);
        if (nextQueueDeadline < nextEventTime) {
            nextEventTime = nextQueueDeadline;
        }

        if (nextEventTime == std::numeric_limits<int>::max()) {
            bool hasUnseatableGroups = false;
            for (const Group& group : queue) {
                if (!hasTableThatFits(tables, group.size)) {
                    hasUnseatableGroups = true;
                    std::cout << "Group " << group.id
                              << " cannot be seated because no table fits size "
                              << group.size << ".\n";
                }
            }

            if (!hasUnseatableGroups && !queue.empty()) {
                std::cout << "Simulation stopped because no future event could be scheduled.\n";
            }
            break;
        }

        currentTime = nextEventTime;
        totalSimulationTime = currentTime;

        for (Table& table : tables) {
            if (!table.isFree && table.availableAt <= currentTime) {
                table.isFree = true;
            }
        }

        while (nextArrivalIdx < totalGroups && orderedArrivals[nextArrivalIdx].arrivalTime <= currentTime) {
            queue.push_back(orderedArrivals[nextArrivalIdx]);
            nextArrivalIdx++;
        }

        processSeating(currentTime);
    }

    if (totalSeatsAvailable == 0 || totalSimulationTime == 0) {
        std::cout << "\nUtilization: 0.00%\n";
        return;
    }

    const double maxPossibleSeatMinutes = totalSeatsAvailable * totalSimulationTime;
    const double utilization = (totalSeatMinutesUsed / maxPossibleSeatMinutes) * 100.0;
    std::cout << "\nUtilization: " << std::fixed << std::setprecision(2) << utilization << "%\n";
}
