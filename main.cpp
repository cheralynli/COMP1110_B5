// main.cpp
// Main program entry point for the restaurant simulation project.
//
// Planned responsibilities:
// - initialize any shared state or default file paths
// - present a simple menu interface for user input
// - let the user choose scenario/config files and simulation options
// - call the simulation engine
// - print or save summary results for the chosen run
// - handle basic input validation and user-facing error messages

#include "parser.h"
#include "simulation.h"

#include <fstream>
#include <iostream>
#include <string>

namespace {

bool canOpenFile(const std::string& path) {
    std::ifstream input(path);
    return input.is_open();
}

std::string resolveInputPath(const std::string& path) {
    if (canOpenFile(path)) {
        return path;
    }

    const std::string project_prefix = "COMP1110_B5/";
    if (path.size() >= project_prefix.size() &&
        path.substr(0, project_prefix.size()) == project_prefix) {
        const std::string stripped_path = path.substr(project_prefix.size());
        if (canOpenFile(stripped_path)) {
            return stripped_path;
        }
    } else {
        const std::string prefixed_path = project_prefix + path;
        if (canOpenFile(prefixed_path)) {
            return prefixed_path;
        }
    }

    return path;
}

} // namespace

int main(int argc, char* argv[]) {
    std::string configPath = "COMP1110_B5/scenario_5_dimsum_kbbq/config.txt";
    std::string arrivalsPath = "COMP1110_B5/scenario_5_dimsum_kbbq/arrivals.txt";

    if (argc == 3) {
        configPath = argv[1];
        arrivalsPath = argv[2];
    } else if (argc != 1) {
        std::cerr << "Usage: restaurant_sim [config_path arrivals_path]\n";
        return 1;
    }

    configPath = resolveInputPath(configPath);
    arrivalsPath = resolveInputPath(arrivalsPath);

    InputParser parser;
    parser.loadConfig(configPath);
    parser.loadArrivals(arrivalsPath);

    const std::vector<Table>& tables = parser.getTables();
    const std::vector<Group>& arrivals = parser.getArrivals();

    if (tables.empty()) {
        std::cerr << "No tables were loaded from " << configPath << ".\n";
        return 1;
    }

    if (arrivals.empty()) {
        std::cerr << "No arrivals were loaded from " << arrivalsPath << ".\n";
        return 1;
    }

    std::cout << "Loaded " << tables.size() << " tables and "
              << arrivals.size() << " arrival groups.\n";

    WokThisWaySim simulation(tables, 1.5, 15);
    simulation.precomputeHourlyRates(arrivals);
    simulation.runSimulation(arrivals);

    return 0;
}
