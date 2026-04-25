#include "restaurant_parser.h"

#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

std::string trimSpace(const std::string& value) {
    std::size_t start = 0;
    while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start])) != 0) {
        start++;
    }

    std::size_t end = value.size();
    while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1])) != 0) {
        end--;
    }

    return value.substr(start, end - start);
}

bool shouldSkipLine(const std::string& rawLine) {
    const std::string line = trimSpace(rawLine);
    return line.empty() || line.front() == '#';
}

std::vector<std::string> splitCsv(const std::string& rawLine) {
    std::vector<std::string> fields;
    std::stringstream stream(rawLine);
    std::string field;

    while (std::getline(stream, field, ',')) {
        fields.push_back(trimSpace(field));
    }

    return fields;
}

bool isIntegerString(const std::string& value) {
    if (value.empty()) {
        return false;
    }

    for (char ch : value) {
        if (ch < '0' || ch > '9') {
            return false;
        }
    }

    return true;
}

bool tryParseInteger(const std::string& value, int& result) {
    if (!isIntegerString(value)) {
        return false;
    }

    try {
        result = std::stoi(value);
    } catch (...) {
        return false;
    }

    return true;
}

void printLineWarning(const std::string& path, int lineNumber, const std::string& message) {
    std::cerr << path << ":" << lineNumber << ": " << message << '\n';
}

bool tableIdExists(const std::vector<Table>& tables, int tableId) {
    for (const Table& table : tables) {
        if (table.id == tableId) {
            return true;
        }
    }

    return false;
}

void InputParser::loadConfig(const std::string& configPath) {
    tables.clear();

    std::ifstream input(configPath);
    if (!input.is_open()) {
        std::cerr << "Could not open config file: " << configPath << '\n';
        return;
    }

    std::string rawLine;
    int lineNumber = 0;
    while (std::getline(input, rawLine)) {
        lineNumber++;
        if (shouldSkipLine(rawLine)) {
            continue;
        }

        const std::vector<std::string> fields = splitCsv(rawLine);
        if (fields.empty()) {
            continue;
        }

        if (fields[0] == "TABLE") {
            if (fields.size() != 3) {
                printLineWarning(configPath, lineNumber, "invalid TABLE line");
                continue;
            }

            int capacity = 0;
            int tableId = 0;

            if (!tryParseInteger(fields[1], capacity) || capacity <= 0) {
                printLineWarning(configPath, lineNumber, "invalid table capacity");
                continue;
            }

            if (!tryParseInteger(fields[2], tableId)) {
                printLineWarning(configPath, lineNumber, "invalid table ID");
                continue;
            }

            if (tableIdExists(tables, tableId)) {
                printLineWarning(configPath, lineNumber, "duplicate table ID");
                continue;
            }

            tables.push_back(Table{tableId, capacity, true, 0});
            continue;
        }

        if (fields[0] == "QUEUE") {
            if (fields.size() != 3) {
                printLineWarning(configPath, lineNumber, "invalid QUEUE line");
                continue;
            }

            int minSize = 0;
            int maxSize = 0;
            if (!tryParseInteger(fields[1], minSize) || !tryParseInteger(fields[2], maxSize) ||
                minSize <= 0 || maxSize <= 0 || minSize > maxSize) {
                printLineWarning(configPath, lineNumber, "invalid queue range");
            }

            continue;
        }

        printLineWarning(configPath, lineNumber, "unknown record type");
    }
}

void InputParser::loadArrivals(const std::string& arrivalsPath) {
    arrivals.clear();

    std::ifstream input(arrivalsPath);
    if (!input.is_open()) {
        std::cerr << "Could not open arrivals file: " << arrivalsPath << '\n';
        return;
    }

    std::string rawLine;
    int nextGroupId = 1;
    int lineNumber = 0;

    while (std::getline(input, rawLine)) {
        lineNumber++;
        if (shouldSkipLine(rawLine)) {
            continue;
        }

        const std::vector<std::string> fields = splitCsv(rawLine);
        if (fields.empty()) {
            continue;
        }

        if (fields[0] != "ARRIVAL") {
            printLineWarning(arrivalsPath, lineNumber, "unknown record type");
            continue;
        }

        if (fields.size() != 4) {
            printLineWarning(arrivalsPath, lineNumber, "invalid ARRIVAL line");
            continue;
        }

        int arrivalTime = 0;
        int groupSize = 0;
        int diningDuration = 0;

        if (!tryParseInteger(fields[1], arrivalTime)) {
            printLineWarning(arrivalsPath, lineNumber, "invalid arrival time");
            continue;
        }

        if (!tryParseInteger(fields[2], groupSize) || groupSize <= 0) {
            printLineWarning(arrivalsPath, lineNumber, "invalid group size");
            continue;
        }

        if (!tryParseInteger(fields[3], diningDuration) || diningDuration <= 0) {
            printLineWarning(arrivalsPath, lineNumber, "invalid dining duration");
            continue;
        }

        int maxWaitTolerance = defaultMaxWaitTolerance;

        // if (fields.size() >= 5) {
        //     maxWaitTolerance = std::stoi(fields[4]);
        // }

        arrivals.push_back(
            Group{nextGroupId, groupSize, arrivalTime, diningDuration, maxWaitTolerance, -1}
        );
        nextGroupId++;
    }
}

void InputParser::setDefaultMaxWaitTolerance(int maxWaitTolerance) {
    if (maxWaitTolerance <= 0) {
        std::cerr << "Default max wait tolerance must be greater than zero.\n";
        return;
    }

    defaultMaxWaitTolerance = maxWaitTolerance;
}

const std::vector<Table>& InputParser::getTables() const {
    return tables;
}

const std::vector<Group>& InputParser::getArrivals() const {
    return arrivals;
}
