#include "size_queue_simulation.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <utility>

namespace {

enum class QueueBucket {
    Small,
    Medium,
    Large,
};

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

bool frontGroupFits(const std::vector<Group>& queue, int tableCapacity) {
    return !queue.empty() && queue.front().size <= tableCapacity;
}

QueueBucket bucketForGroupSize(int groupSize) {
    if (groupSize <= 2) {
        return QueueBucket::Small;
    }

    if (groupSize <= 4) {
        return QueueBucket::Medium;
    }

    return QueueBucket::Large;
}

std::vector<Group>* selectQueueForTable(
    Table& table,
    std::vector<Group>& smallQueue,
    std::vector<Group>& mediumQueue,
    std::vector<Group>& largeQueue
) {
    std::vector<Group>* queueOrder[3] = {nullptr, nullptr, nullptr};

    if (table.capacity <= 2) {
        queueOrder[0] = &smallQueue;
        queueOrder[1] = &mediumQueue;
        queueOrder[2] = &largeQueue;
    } else if (table.capacity <= 4) {
        queueOrder[0] = &mediumQueue;
        queueOrder[1] = &smallQueue;
        queueOrder[2] = &largeQueue;
    } else {
        queueOrder[0] = &largeQueue;
        queueOrder[1] = &mediumQueue;
        queueOrder[2] = &smallQueue;
    }

    for (std::vector<Group>* queue : queueOrder) {
        if (queue != nullptr && frontGroupFits(*queue, table.capacity)) {
            return queue;
        }
    }

    return nullptr;
}

void warnUnseatableGroup(const Group& group) {
    std::cout << "Group " << group.id
              << " cannot be seated because no table fits size "
              << group.size << ".\n";
}

} // namespace

SizeQueueSimulation::SizeQueueSimulation(std::vector<Table> inputTables) {
    tables = std::move(inputTables);
    std::sort(tables.begin(), tables.end(), compareTablesForSeating);

    for (const Table& table : tables) {
        totalSeatsAvailable += table.capacity;
    }
}

void SizeQueueSimulation::setSeatingLogPath(const std::string& path) {
    seatingLogPath = path;
}

void SizeQueueSimulation::resetState() {
    smallQueue.clear();
    mediumQueue.clear();
    largeQueue.clear();
    totalSeatMinutesUsed = 0;
    totalSimulationTime = 0;

    for (Table& table : tables) {
        table.isFree = true;
        table.availableAt = 0;
    }
}

void SizeQueueSimulation::initializeSeatingLog() const {
    std::ofstream output(seatingLogPath);
    if (!output.is_open()) {
        return;
    }

    output << "group_id,group_size,arrival_time,seating_time,wait_time,table_id,"
           << "table_capacity,dining_duration,departure_time\n";
}

void SizeQueueSimulation::appendSeatingRecord(const Group& group, const Table& table) {
    std::ofstream output(seatingLogPath, std::ios::app);
    if (!output.is_open()) {
        return;
    }

    const int waitTime = group.seatingTime - group.arrivalTime;
    const int departureTime = group.seatingTime + group.diningDuration;

    output << group.id << ','
           << group.size << ','
           << group.arrivalTime << ','
           << group.seatingTime << ','
           << waitTime << ','
           << table.id << ','
           << table.capacity << ','
           << group.diningDuration << ','
           << departureTime << '\n';
}

void SizeQueueSimulation::enqueueGroup(const Group& group) {
    switch (bucketForGroupSize(group.size)) {
        case QueueBucket::Small:
            smallQueue.push_back(group);
            break;
        case QueueBucket::Medium:
            mediumQueue.push_back(group);
            break;
        case QueueBucket::Large:
            largeQueue.push_back(group);
            break;
    }
}

bool SizeQueueSimulation::hasWaitingGroups() const {
    return !smallQueue.empty() || !mediumQueue.empty() || !largeQueue.empty();
}

void SizeQueueSimulation::processSeating(int currentTime) {
    bool seatedSomeone = true;
    while (seatedSomeone) {
        seatedSomeone = false;

        for (Table& table : tables) {
            if (!table.isFree) {
                continue;
            }

            std::vector<Group>* selectedQueue =
                selectQueueForTable(table, smallQueue, mediumQueue, largeQueue);
            if (selectedQueue == nullptr) {
                continue;
            }

            Group group = selectedQueue->front();
            group.seatingTime = currentTime;
            table.isFree = false;
            table.availableAt = currentTime + group.diningDuration;
            totalSeatMinutesUsed += group.size * group.diningDuration;
            appendSeatingRecord(group, table);

            std::cout << "Time " << std::setw(3) << currentTime
                      << " | Group " << std::setw(2) << group.id
                      << " (Size " << group.size << ") @ Table " << table.id << "\n";

            selectedQueue->erase(selectedQueue->begin());
            seatedSomeone = true;
            break;
        }
    }
}

void SizeQueueSimulation::runSimulation(const std::vector<Group>& arrivals) {
    resetState();
    initializeSeatingLog();

    std::vector<Group> orderedArrivals = arrivals;
    std::sort(orderedArrivals.begin(), orderedArrivals.end(), compareGroupsByArrival);

    int nextArrivalIdx = 0;
    int currentTime = 0;
    const int totalGroups = static_cast<int>(orderedArrivals.size());

    while (nextArrivalIdx < totalGroups || hasWaitingGroups() || hasOccupiedTables(tables)) {
        int nextEventTime = std::numeric_limits<int>::max();

        if (nextArrivalIdx < totalGroups) {
            nextEventTime = orderedArrivals[static_cast<std::size_t>(nextArrivalIdx)].arrivalTime;
        }

        for (const Table& table : tables) {
            if (!table.isFree && table.availableAt < nextEventTime) {
                nextEventTime = table.availableAt;
            }
        }

        if (nextEventTime == std::numeric_limits<int>::max()) {
            bool hasUnseatableGroups = false;
            const std::vector<const std::vector<Group>*> allQueues = {
                &smallQueue,
                &mediumQueue,
                &largeQueue,
            };

            for (const std::vector<Group>* queue : allQueues) {
                for (const Group& group : *queue) {
                    if (!hasTableThatFits(tables, group.size)) {
                        hasUnseatableGroups = true;
                        warnUnseatableGroup(group);
                    }
                }
            }

            if (!hasUnseatableGroups && hasWaitingGroups()) {
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

        while (nextArrivalIdx < totalGroups &&
               orderedArrivals[static_cast<std::size_t>(nextArrivalIdx)].arrivalTime <= currentTime) {
            const Group& group = orderedArrivals[static_cast<std::size_t>(nextArrivalIdx)];
            if (!hasTableThatFits(tables, group.size)) {
                warnUnseatableGroup(group);
                nextArrivalIdx++;
                continue;
            }

            enqueueGroup(group);
            nextArrivalIdx++;
        }

        processSeating(currentTime);
    }

    if (totalSeatsAvailable == 0 || totalSimulationTime == 0) {
        std::cout << "\nUtilization: 0.00%\n";
        return;
    }

    const double maxPossibleSeatMinutes =
        static_cast<double>(totalSeatsAvailable) * static_cast<double>(totalSimulationTime);
    const double utilization =
        (static_cast<double>(totalSeatMinutesUsed) / maxPossibleSeatMinutes) * 100.0;
    std::cout << "\nUtilization: " << std::fixed << std::setprecision(2) << utilization << "%\n";
}
