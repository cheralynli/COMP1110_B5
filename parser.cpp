#include "parser.h"

#include <cctype>
#include <fstream>
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

void InputParser::loadConfig(const std::string& configPath) {
    std::ifstream input(configPath);
    if (!input.is_open()) {
        return;
    }

    tables.clear();

    std::string rawLine;
    while (std::getline(input, rawLine)) {
        if (shouldSkipLine(rawLine)) {
            continue;
        }

        const std::vector<std::string> fields = splitCsv(rawLine);
        if (fields.empty()) {
            continue;
        }

        if (fields[0] == "TABLE") {
            const int capacity = std::stoi(fields[1]);
            if (!isIntegerString(fields[2])) {
                continue;
            }

            const int tableId = std::stoi(fields[2]);
            tables.push_back(Table{tableId, capacity, true, 0});
            continue;
        }
    }
}

void InputParser::loadArrivals(const std::string& arrivalsPath) {
    std::ifstream input(arrivalsPath);
    if (!input.is_open()) {
        return;
    }

    arrivals.clear();

    std::string rawLine;
    int nextGroupId = 1;

    while (std::getline(input, rawLine)) {
        if (shouldSkipLine(rawLine)) {
            continue;
        }

        const std::vector<std::string> fields = splitCsv(rawLine);
        if (fields.empty()) {
            continue;
        }

        if (fields[0] != "ARRIVAL") {
            continue;
        }

        const int arrival_time = std::stoi(fields[1]);
        const int group_size = std::stoi(fields[2]);
        const int dining_duration = std::stoi(fields[3]);
        int maxWaitTolerance = defaultMaxWaitTolerance;

        // if (fields.size() >= 5) {
        //     maxWaitTolerance = std::stoi(fields[4]);
        // }

        arrivals.push_back(
            Group{nextGroupId, group_size, arrival_time, dining_duration, maxWaitTolerance, -1}
        );
        nextGroupId++;
    }
}

void InputParser::setDefaultMaxWaitTolerance(int maxWaitTolerance) {
    defaultMaxWaitTolerance = maxWaitTolerance;
}

const std::vector<Table>& InputParser::getTables() const {
    return tables;
}

const std::vector<Group>& InputParser::getArrivals() const {
    return arrivals;
}
