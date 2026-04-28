#include "fcfs_simulation.h"

#include <algorithm>
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

void warnUnseatableGroup(const Group& group) {
    std::cout << "Group " << group.id
              << " cannot be seated because no table fits size "
              << group.size << ".\n";
}

} // namespace

FCFSSimulation::FCFSSimulation(std::vector<Table> inputTables) {
    tables = std::move(inputTables);
    std::sort(tables.begin(), tables.end(), compareTablesForSeating);

    for (const Table& table : tables) {
        totalSeatsAvailable += table.capacity;
    }
}

void FCFSSimulation::setSeatingLogPath(const std::string& path) {
    seatingLogPath = path;
}

void FCFSSimulation::resetState() {
    queue.clear();
    totalSeatMinutesUsed = 0;
    totalSimulationTime = 0;

    for (Table& table : tables) {
        table.isFree = true;
        table.availableAt = 0;
    }
}

void FCFSSimulation::initializeSeatingLog() const {
    std::ofstream output(seatingLogPath);
    if (!output.is_open()) {
        return;
    }

    output << "group_id,group_size,arrival_time,seating_time,wait_time,table_id,"
           << "table_capacity,dining_duration,departure_time\n";
}

void FCFSSimulation::appendSeatingRecord(const Group& group, const Table& table) {
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

void FCFSSimulation::processSeating(int currentTime) {
    bool seatedSomeone = true;
    while (seatedSomeone) {
        seatedSomeone = false;

        for (std::size_t queueIndex = 0; queueIndex < queue.size(); ++queueIndex) {
            Table* selectedTable = nullptr;
            for (Table& table : tables) {
                if (!table.isFree || table.capacity < queue[queueIndex].size) {
                    continue;
                }

                selectedTable = &table;
                break;
            }

            if (selectedTable == nullptr) {
                continue;
            }

            Group group = queue[queueIndex];
            group.seatingTime = currentTime;
            selectedTable->isFree = false;
            selectedTable->availableAt = currentTime + group.diningDuration;
            totalSeatMinutesUsed += group.size * group.diningDuration;
            appendSeatingRecord(group, *selectedTable);

            std::cout << "Time " << std::setw(3) << currentTime
                      << " | Group " << std::setw(2) << group.id
                      << " (Size " << group.size << ") @ Table " << selectedTable->id << "\n";

            queue.erase(queue.begin() + static_cast<std::ptrdiff_t>(queueIndex));
            seatedSomeone = true;
            break;
        }
    }
}

void FCFSSimulation::runSimulation(const std::vector<Group>& arrivals) {
    resetState();
    initializeSeatingLog();

    std::vector<Group> orderedArrivals = arrivals;
    std::sort(orderedArrivals.begin(), orderedArrivals.end(), compareGroupsByArrival);

    int nextArrivalIdx = 0;
    int currentTime = 0;
    const int totalGroups = static_cast<int>(orderedArrivals.size());

    while (nextArrivalIdx < totalGroups || !queue.empty() || hasOccupiedTables(tables)) {
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
            for (const Group& group : queue) {
                if (!hasTableThatFits(tables, group.size)) {
                    hasUnseatableGroups = true;
                    warnUnseatableGroup(group);
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

        while (nextArrivalIdx < totalGroups &&
               orderedArrivals[static_cast<std::size_t>(nextArrivalIdx)].arrivalTime <= currentTime) {
            const Group& group = orderedArrivals[static_cast<std::size_t>(nextArrivalIdx)];
            if (!hasTableThatFits(tables, group.size)) {
                warnUnseatableGroup(group);
                nextArrivalIdx++;
                continue;
            }

            queue.push_back(group);
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
