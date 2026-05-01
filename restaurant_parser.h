#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <vector>

struct Group {
    int id = 0;
    int size = 0;
    int arrivalTime = 0;
    int diningDuration = 0;
    int maxWaitTolerance = 30;
    int seatingTime = -1;
};

struct Table {
    int id = 0;
    int capacity = 0;
    bool isFree = true;
    int availableAt = 0;
};

struct QueueRule {
    int minSize = 1;
    int maxSize = 99;
};

class InputParser {
public:
    void loadConfig(const std::string& configPath);
    void loadArrivals(const std::string& arrivalsPath);
    void setDefaultMaxWaitTolerance(int maxWaitTolerance);

    const std::vector<Table>& getTables() const;
    const std::vector<Group>& getArrivals() const;
    const std::vector<QueueRule>& getQueueRules() const;

private:
    std::vector<Table> tables;
    std::vector<Group> arrivals;
    std::vector<QueueRule> queueRules;
    int defaultMaxWaitTolerance = 30;
};

#endif
