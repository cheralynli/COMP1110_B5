#include "simulation.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <utility>

#ifndef DEPARTURE_REGRET_WEIGHT
#define DEPARTURE_REGRET_WEIGHT 0.04
#endif

namespace {

bool compareGroupsByArrival(const Group& left, const Group& right) {
    if (left.arrivalTime != right.arrivalTime) {
        return left.arrivalTime < right.arrivalTime;
    }

    return left.id < right.id;
}

bool compareTablesForSeating(const Table& left, const Table& right) {
    if (left.capacity != right.capacity) {
        return left.capacity < right.capacity;
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

std::map<int, double> expectedDiningByGroupSize;
std::map<int, int> maxDiningByGroupSize;
double defaultExpectedDiningMinutes = 45.0;
std::map<int, int> estimatedTableAvailableAt;

double expectedDiningForSize(int groupSize) {
    const auto exact = expectedDiningByGroupSize.find(groupSize);
    if (exact != expectedDiningByGroupSize.end()) {
        return exact->second;
    }

    return defaultExpectedDiningMinutes;
}

int maxDiningForSize(int groupSize) {
    const auto exact = maxDiningByGroupSize.find(groupSize);
    if (exact != maxDiningByGroupSize.end()) {
        return exact->second;
    }

    return static_cast<int>(std::round(defaultExpectedDiningMinutes));
}

int conservativeDiningEstimateForSize(int groupSize) {
    const int maxDining = maxDiningForSize(groupSize);
    if (maxDining > 0) {
        return maxDining;
    }

    const double expectedDining = expectedDiningForSize(groupSize);
    return std::max(1, static_cast<int>(std::ceil(expectedDining * 1.25)));
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
    if (algorithm == AlgorithmType::Custom) {
        std::sort(tables.begin(), tables.end(), compareTablesForSeating);
    }

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
    estimatedTableAvailableAt.clear();

    for (auto& table : tables) {
        table.isFree = true;
        table.availableAt = 0;
        estimatedTableAvailableAt[table.id] = 0;
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
    expectedDiningByGroupSize.clear();
    maxDiningByGroupSize.clear();
    defaultExpectedDiningMinutes = 45.0;

    std::map<int, std::map<int, int>> hourlyCounts;
    std::map<int, int> diningDurationTotals;
    std::map<int, int> diningDurationCounts;
    int overallDiningTotal = 0;
    int overallDiningCount = 0;

    for (const Group& group : historicalData) {
        const int arrival_hour = group.arrivalTime / 60;
        hourlyCounts[arrival_hour][group.size]++;
        diningDurationTotals[group.size] += group.diningDuration;
        diningDurationCounts[group.size]++;
        maxDiningByGroupSize[group.size] =
            std::max(maxDiningByGroupSize[group.size], group.diningDuration);
        overallDiningTotal += group.diningDuration;
        overallDiningCount++;
    }

    if (overallDiningCount > 0) {
        defaultExpectedDiningMinutes =
            static_cast<double>(overallDiningTotal) / static_cast<double>(overallDiningCount);
    }

    for (const auto& countPair : diningDurationCounts) {
        const int size = countPair.first;
        const int count = countPair.second;
        if (count > 0) {
            expectedDiningByGroupSize[size] =
                static_cast<double>(diningDurationTotals[size]) / static_cast<double>(count);
        }
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
    if (algorithm == AlgorithmType::Custom) {
        const double fairnessScale = std::clamp(fairnessWeight, 0.25, 3.0);
        const int effectiveLookAhead = std::clamp(lookAheadWindow, 5, 20);

        auto getCurrentArrivalRates = [&]() -> const std::map<int, double>* {
            if (hourlyArrivalRates.empty()) {
                return nullptr;
            }

            const int currentHour = currentTime / 60;
            auto exact = hourlyArrivalRates.find(currentHour);
            if (exact != hourlyArrivalRates.end()) {
                return &exact->second;
            }

            auto after = hourlyArrivalRates.upper_bound(currentHour);
            if (after == hourlyArrivalRates.begin()) {
                return &after->second;
            }

            --after;
            return &after->second;
        };

        auto minutesUntilAlternativeTable = [&](const Table& candidateTable, int futureGroupSize) {
            double bestDelay = std::numeric_limits<double>::infinity();

            for (const Table& otherTable : tables) {
                if (otherTable.id == candidateTable.id || otherTable.capacity < futureGroupSize) {
                    continue;
                }

                if (otherTable.isFree) {
                    return 0.0;
                }

                const auto estimated = estimatedTableAvailableAt.find(otherTable.id);
                const int estimatedAvailableAt =
                    (estimated == estimatedTableAvailableAt.end())
                        ? otherTable.availableAt
                        : estimated->second;
                const double delay = static_cast<double>(
                    std::max(0, estimatedAvailableAt - currentTime)
                );
                if (delay < bestDelay) {
                    bestDelay = delay;
                }
            }

            return bestDelay;
        };

        auto expectedMismatchRegret = [&](const Table& table, const Group& group) {
            const std::map<int, double>* currentRates = getCurrentArrivalRates();
            if (currentRates == nullptr) {
                return 0.0;
            }

            double regret = 0.0;
            for (const auto& ratePair : *currentRates) {
                const int futureGroupSize = ratePair.first;
                const double lambda = ratePair.second;

                if (futureGroupSize <= group.size || futureGroupSize > table.capacity) {
                    continue;
                }

                const double alternativeDelay = minutesUntilAlternativeTable(table, futureGroupSize);
                const double currentGroupBlockingTime =
                    static_cast<double>(conservativeDiningEstimateForSize(group.size));
                double blockingWindow = std::min(
                    static_cast<double>(effectiveLookAhead),
                    currentGroupBlockingTime
                );
                if (std::isfinite(alternativeDelay)) {
                    blockingWindow = std::min(blockingWindow, alternativeDelay);
                }

                if (blockingWindow <= 0.0) {
                    continue;
                }

                const double arrivalProbability = 1.0 - std::exp(-lambda * blockingWindow);
                const double extraSeatsLost = static_cast<double>(futureGroupSize - group.size);
                const double futureDiningValue =
                    static_cast<double>(conservativeDiningEstimateForSize(futureGroupSize));

                regret += arrivalProbability * extraSeatsLost * futureDiningValue;
            }

            return regret;
        };

        auto smallestFreeTableCapacityFor = [&](int groupSize) {
            int bestCapacity = std::numeric_limits<int>::max();
            for (const Table& table : tables) {
                if (table.isFree && table.capacity >= groupSize && table.capacity < bestCapacity) {
                    bestCapacity = table.capacity;
                }
            }

            return bestCapacity;
        };

        auto scorePair = [&](const Table& table, const Group& group, int queueIndex) {
            const int waitTime = std::max(0, currentTime - group.arrivalTime);
            const int tolerance = std::max(1, group.maxWaitTolerance);
            const int serviceTime = conservativeDiningEstimateForSize(group.size);
            const int unusedSeats = table.capacity - group.size;
            const int smallestCapacity = smallestFreeTableCapacityFor(group.size);
            const int extraCapacity =
                (smallestCapacity == std::numeric_limits<int>::max())
                    ? 0
                    : std::max(0, table.capacity - smallestCapacity);

            const double responseRatio =
                1.0 + (static_cast<double>(waitTime) / static_cast<double>(serviceTime));
            const double urgencyRatio = std::min(
                3.0,
                static_cast<double>(waitTime) / static_cast<double>(tolerance)
            );
            const double fillRatio =
                static_cast<double>(group.size) / static_cast<double>(table.capacity);
            const double futureRegret = expectedMismatchRegret(table, group);

            return (8.00 * fairnessScale * static_cast<double>(waitTime)) +
                   (55.00 * fairnessScale * responseRatio) +
                   (78.00 * fairnessScale * urgencyRatio) +
                   (55.00 * fillRatio) +
                   (13.00 * static_cast<double>(group.size)) -
                   (2.00 * static_cast<double>(serviceTime)) -
                   (0.3 * static_cast<double>(unusedSeats * unusedSeats)) -
                   (22.00 * static_cast<double>(extraCapacity * extraCapacity)) -
                   (DEPARTURE_REGRET_WEIGHT * futureRegret) -
                   (2.00 * fairnessScale * static_cast<double>(queueIndex));
        };

        while (true) {
            int bestTableIdx = -1;
            int bestQueueIdx = -1;
            double bestScore = -std::numeric_limits<double>::infinity();

            for (int tableIdx = 0; tableIdx < static_cast<int>(tables.size()); ++tableIdx) {
                const Table& table = tables[static_cast<std::size_t>(tableIdx)];
                if (!table.isFree) {
                    continue;
                }

                for (int queueIdx = 0; queueIdx < static_cast<int>(queue.size()); ++queueIdx) {
                    const Group& group = queue[static_cast<std::size_t>(queueIdx)];
                    if (group.size > table.capacity) {
                        continue;
                    }

                    const double score = scorePair(table, group, queueIdx);
                    bool better = score > bestScore;
                    if (!better && std::abs(score - bestScore) < 1e-9 && bestQueueIdx >= 0) {
                        const Group& previousGroup = queue[static_cast<std::size_t>(bestQueueIdx)];
                        const Table& previousTable = tables[static_cast<std::size_t>(bestTableIdx)];

                        if (group.arrivalTime != previousGroup.arrivalTime) {
                            better = group.arrivalTime < previousGroup.arrivalTime;
                        } else if (table.capacity != previousTable.capacity) {
                            better = table.capacity < previousTable.capacity;
                        } else if (group.id != previousGroup.id) {
                            better = group.id < previousGroup.id;
                        } else {
                            better = table.id < previousTable.id;
                        }
                    }

                    if (better) {
                        bestScore = score;
                        bestTableIdx = tableIdx;
                        bestQueueIdx = queueIdx;
                    }
                }
            }

            if (bestTableIdx == -1 || bestQueueIdx == -1) {
                break;
            }

            Table& selectedTable = tables[static_cast<std::size_t>(bestTableIdx)];
            Group selectedGroup = queue[static_cast<std::size_t>(bestQueueIdx)];

            selectedTable.isFree = false;
            selectedTable.availableAt = currentTime + selectedGroup.diningDuration;
            estimatedTableAvailableAt[selectedTable.id] =
                currentTime + conservativeDiningEstimateForSize(selectedGroup.size);
            selectedGroup.seatingTime = currentTime;
            totalSeatMinutesUsed += selectedGroup.size * selectedGroup.diningDuration;

            const int waitTime = selectedGroup.seatingTime - selectedGroup.arrivalTime;
            totalWaitTime += waitTime;
            maxWaitTime = std::max(maxWaitTime, waitTime);
            groupsServed++;
            if (waitTime <= 15) {
                groupsWithin15++;
            }

            appendSeatingRecord(selectedGroup, selectedTable);

            std::cout << "Time " << std::setw(3) << currentTime
                      << " | Group " << std::setw(2) << selectedGroup.id
                      << " (Size " << selectedGroup.size << ") @ Table "
                      << selectedTable.id << "\n";

            queue.erase(queue.begin() + bestQueueIdx);
        }

        return;
    }

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
                if (algorithm == AlgorithmType::Custom) {
                    estimatedTableAvailableAt[table.id] = currentTime;
                }
            }
        }

        while (nextArrivalIdx < totalGroups && orderedArrivals[nextArrivalIdx].arrivalTime <= currentTime) {
            const Group& arrivingGroup = orderedArrivals[static_cast<std::size_t>(nextArrivalIdx)];
            if (algorithm == AlgorithmType::Custom && !hasTableThatFits(tables, arrivingGroup.size)) {
                std::cout << "Group " << arrivingGroup.id
                          << " cannot be seated because no table fits size "
                          << arrivingGroup.size << ".\n";
                nextArrivalIdx++;
                continue;
            }

            queue.push_back(arrivingGroup);
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
