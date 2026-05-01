#include "simulation.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <utility>

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

WokThisWaySim::WokThisWaySim(
    std::vector<Table> inputTables,
    std::vector<QueueRule> inputQueueRules,
    AlgorithmType selectedAlgorithm,
    double alpha,
    int window
) {
    tables = std::move(inputTables);
    queueRules = std::move(inputQueueRules);
    algorithm = selectedAlgorithm;
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
    totalWaitTime = 0;
    maxWaitTime = 0;
    groupsServed = 0;
    groupsWithin15 = 0;
    maxQueueLength = 0;

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

double WokThisWaySim::calculateOpportunityCost(int tableCapacity, int currentTime) const {
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

            const int bestIdx = selectNextGroupIndex(table, currentTime);
            if (bestIdx < 0) {
                continue;
            }

            Group bestGroup = queue[bestIdx];
            table.isFree = false;
            table.availableAt = currentTime + bestGroup.diningDuration;
            bestGroup.seatingTime = currentTime;
            totalSeatMinutesUsed += bestGroup.size * bestGroup.diningDuration;
            const int waitTime = bestGroup.seatingTime - bestGroup.arrivalTime;
            totalWaitTime += waitTime;
            maxWaitTime = std::max(maxWaitTime, waitTime);
            groupsServed++;
            if (waitTime <= 15) {
                groupsWithin15++;
            }
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

int WokThisWaySim::selectCustomGroupIndex(const Table& table, int currentTime) const {
    int bestIdx = -1;
    double bestUtility = -1.0;

    for (int i = 0; i < static_cast<int>(queue.size()); ++i) {
        if (queue[i].size > table.capacity) {
            continue;
        }

        const int currentWaitTime = currentTime - queue[i].arrivalTime;
        const double utility =
            (queue[i].size * queue[i].diningDuration) + (currentWaitTime * fairnessWeight);

        if (utility > bestUtility) {
            bestUtility = utility;
            bestIdx = i;
        }
    }

    if (bestIdx < 0) {
        return -1;
    }

    const Group& bestGroup = queue[bestIdx];
    const int currentWaitTime = currentTime - bestGroup.arrivalTime;
    const double expectedValueWait = calculateOpportunityCost(table.capacity, currentTime);
    if (bestUtility <= expectedValueWait && currentWaitTime < bestGroup.maxWaitTolerance) {
        return -1;
    }

    return bestIdx;
}

int WokThisWaySim::selectFcfsGroupIndex(const Table& table) const {
    int bestIdx = -1;

    for (int i = 0; i < static_cast<int>(queue.size()); ++i) {
        if (queue[i].size > table.capacity) {
            continue;
        }

        if (bestIdx < 0 ||
            queue[i].arrivalTime < queue[bestIdx].arrivalTime ||
            (queue[i].arrivalTime == queue[bestIdx].arrivalTime && queue[i].id < queue[bestIdx].id)) {
            bestIdx = i;
        }
    }

    return bestIdx;
}

int WokThisWaySim::selectSizeQueueGroupIndex(const Table& table) const {
    std::vector<QueueRule> eligibleRules;
    for (const QueueRule& rule : queueRules) {
        if (rule.minSize <= table.capacity) {
            eligibleRules.push_back(rule);
        }
    }

    std::sort(eligibleRules.begin(), eligibleRules.end(), [](const QueueRule& left, const QueueRule& right) {
        if (left.maxSize != right.maxSize) {
            return left.maxSize > right.maxSize;
        }

        return left.minSize > right.minSize;
    });

    for (const QueueRule& rule : eligibleRules) {
        int bestIdx = -1;
        for (int i = 0; i < static_cast<int>(queue.size()); ++i) {
            if (queue[i].size < rule.minSize || queue[i].size > rule.maxSize) {
                continue;
            }
            if (queue[i].size > table.capacity) {
                continue;
            }

            if (bestIdx < 0 ||
                queue[i].arrivalTime < queue[bestIdx].arrivalTime ||
                (queue[i].arrivalTime == queue[bestIdx].arrivalTime && queue[i].id < queue[bestIdx].id)) {
                bestIdx = i;
            }
        }

        if (bestIdx >= 0) {
            return bestIdx;
        }
    }

    return -1;
}

int WokThisWaySim::selectNextGroupIndex(const Table& table, int currentTime) const {
    if (algorithm == AlgorithmType::FCFS) {
        return selectFcfsGroupIndex(table);
    }

    if (algorithm == AlgorithmType::SizeQueue) {
        return selectSizeQueueGroupIndex(table);
    }

    return selectCustomGroupIndex(table, currentTime);
}

SimulationSummary WokThisWaySim::runSimulation(const std::vector<Group>& arrivals) {
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

        if (algorithm == AlgorithmType::Custom) {
            const int nextQueueDeadline = findNextQueueDeadline(queue, tables, currentTime);
            if (nextQueueDeadline < nextEventTime) {
                nextEventTime = nextQueueDeadline;
            }
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

        maxQueueLength = std::max(maxQueueLength, static_cast<int>(queue.size()));
        processSeating(currentTime);
    }

    SimulationSummary summary;
    summary.groupsServed = groupsServed;
    if (groupsServed > 0) {
        summary.averageWait = static_cast<double>(totalWaitTime) / static_cast<double>(groupsServed);
        summary.serviceLevel15 =
            (static_cast<double>(groupsWithin15) / static_cast<double>(groupsServed)) * 100.0;
    }
    summary.maxWait = maxWaitTime;
    summary.maxQueueLength = maxQueueLength;
    summary.totalSimulationTime = totalSimulationTime;

    if (totalSeatsAvailable == 0 || totalSimulationTime == 0) {
        std::cout << "\nUtilization: 0.00%\n";
        summary.tableUtilization = 0.0;
        return summary;
    }

    const double maxPossibleSeatMinutes = totalSeatsAvailable * totalSimulationTime;
    const double utilization = (totalSeatMinutesUsed / maxPossibleSeatMinutes) * 100.0;
    std::cout << "\nUtilization: " << std::fixed << std::setprecision(2) << utilization << "%\n";
    summary.tableUtilization = utilization;
    return summary;
}
