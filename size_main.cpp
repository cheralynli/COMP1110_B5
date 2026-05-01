#include "restaurant_parser.h"
#include "size_queue_simulation.h"

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace {

struct ScenarioOption {
    std::string name;
    std::string restaurantType;
    std::string demandLevel;
    std::string description;
    std::string configPath;
    std::string arrivalsPath;
};

struct RestaurantOption {
    std::string key;
    std::string label;
    std::string description;
};

const std::vector<RestaurantOption> kRestaurants = {
    {"fastfood", "Fast Food", "Small groups, quick turnover, short dining durations"},
    {"cafe", "Cafe", "Small-to-medium groups with moderate dining durations"},
    {"family", "Family Dining", "Larger groups with longer stays and mixed table sizes"},
};

const std::vector<ScenarioOption> kScenarios = {
    {
        "fastfood_non_peak",
        "fastfood",
        "non_peak",
        "Fast food with light traffic and quick turnover",
        "input/fastfood_non_peak/config.txt",
        "input/fastfood_non_peak/arrivals.txt",
    },
    {
        "fastfood_peak",
        "fastfood",
        "peak",
        "Fast food rush with dense small-party arrivals",
        "input/fastfood_peak/config.txt",
        "input/fastfood_peak/arrivals.txt",
    },
    {
        "cafe_non_peak",
        "cafe",
        "non_peak",
        "Cafe with lighter traffic and moderate dining durations",
        "input/cafe_non_peak/config.txt",
        "input/cafe_non_peak/arrivals.txt",
    },
    {
        "cafe_peak",
        "cafe",
        "peak",
        "Cafe rush with more queue pressure",
        "input/cafe_peak/config.txt",
        "input/cafe_peak/arrivals.txt",
    },
    {
        "family_non_peak",
        "family",
        "non_peak",
        "Family dining with moderate traffic and larger groups",
        "input/family_non_peak/config.txt",
        "input/family_non_peak/arrivals.txt",
    },
    {
        "family_peak",
        "family",
        "peak",
        "Family dining rush with many medium and large groups",
        "input/family_peak/config.txt",
        "input/family_peak/arrivals.txt",
    },
};

int customScenarioChoice() {
    return static_cast<int>(kRestaurants.size()) + 1;
}

int runAllScenariosChoice() {
    return static_cast<int>(kRestaurants.size()) + 2;
}

int promptForChoice(const std::string& prompt, int minChoice, int maxChoice) {
    while (true) {
        std::cout << prompt;

        std::string line;
        std::getline(std::cin, line);

        try {
            const int choice = std::stoi(line);
            if (choice >= minChoice && choice <= maxChoice) {
                return choice;
            }
        } catch (...) {
        }

        std::cout << "Invalid choice. Please enter a number from the menu.\n";
    }
}

int promptForRestaurantChoice() {
    std::cout << "\nSize-based queue simulation\n";
    std::cout << "1. Fast Food    - " << kRestaurants[0].description << '\n';
    std::cout << "2. Cafe         - " << kRestaurants[1].description << '\n';
    std::cout << "3. Family Dining - " << kRestaurants[2].description << '\n';
    std::cout << customScenarioChoice() << ". Custom file paths\n";
    std::cout << runAllScenariosChoice() << ". Run all built-in scenarios\n";
    std::cout << "0. Exit\n\n";
    return promptForChoice("Choose a restaurant: ", 0, runAllScenariosChoice());
}

int promptForDemandChoice(const RestaurantOption& restaurant) {
    std::cout << "\nChoose demand level for " << restaurant.label << '\n';
    std::cout << "1. Non-peak\n";
    std::cout << "2. Peak\n";
    std::cout << "0. Back\n\n";
    return promptForChoice("Choose demand level: ", 0, 2);
}

ScenarioOption promptForCustomScenario() {
    ScenarioOption option;
    option.name = "custom_run";
    option.description = "Custom input paths";

    std::cout << "\nCustom scenario input\n";
    std::cout << "Enter config file path: ";
    std::getline(std::cin, option.configPath);
    std::cout << "Enter arrivals file path: ";
    std::getline(std::cin, option.arrivalsPath);

    return option;
}

const ScenarioOption* findScenario(const std::string& restaurantType, const std::string& demandLevel) {
    for (const ScenarioOption& scenario : kScenarios) {
        if (scenario.restaurantType == restaurantType && scenario.demandLevel == demandLevel) {
            return &scenario;
        }
    }

    return nullptr;
}

std::string buildLogPath(const std::string& scenarioName) {
    return "output/size_seating_log_" + scenarioName + ".csv";
}

void runScenario(const ScenarioOption& scenario, bool pauseAfterRun = true) {
    InputParser parser;
    parser.loadConfig(scenario.configPath);
    parser.loadArrivals(scenario.arrivalsPath);

    const std::vector<Table>& tables = parser.getTables();
    const std::vector<Group>& arrivals = parser.getArrivals();

    if (tables.empty()) {
        std::cout << "No tables were loaded from " << scenario.configPath << ".\n";
        return;
    }

    if (arrivals.empty()) {
        std::cout << "No arrivals were loaded from " << scenario.arrivalsPath << ".\n";
        return;
    }

    SizeQueueSimulation simulation(tables);
    const std::string logPath = buildLogPath(scenario.name);
    std::filesystem::create_directories("output");
    simulation.setSeatingLogPath(logPath);

    std::cout << "\nRunning size-based queue simulation for " << scenario.name << '\n';
    std::cout << "Config:   " << scenario.configPath << '\n';
    std::cout << "Arrivals: " << scenario.arrivalsPath << '\n';
    std::cout << "Log file: " << logPath << "\n\n";

    simulation.runSimulation(arrivals);

    std::cout << "\nSimulation complete. Seating log saved to " << logPath << ".\n";
    if (pauseAfterRun) {
        std::cout << "Press Enter to return to the menu.";
        std::string line;
        std::getline(std::cin, line);
    }
}

void runAllScenarios() {
    std::cout << "\nRunning all built-in size-based scenarios\n\n";

    int successCount = 0;
    for (const ScenarioOption& scenario : kScenarios) {
        InputParser parser;
        parser.loadConfig(scenario.configPath);
        parser.loadArrivals(scenario.arrivalsPath);
        if (parser.getTables().empty() || parser.getArrivals().empty()) {
            runScenario(scenario, false);
        } else {
            runScenario(scenario, false);
            successCount++;
        }
        std::cout << '\n';
    }

    std::cout << "Completed " << successCount << " of "
              << static_cast<int>(kScenarios.size()) << " built-in scenarios.\n";
    std::cout << "Press Enter to return to the menu.";
    std::string line;
    std::getline(std::cin, line);
}

} // namespace

int main() {
    while (true) {
        const int restaurantChoice = promptForRestaurantChoice();
        if (restaurantChoice == 0) {
            std::cout << "Exiting.\n";
            return 0;
        }

        ScenarioOption selectedScenario;
        if (restaurantChoice == customScenarioChoice()) {
            selectedScenario = promptForCustomScenario();
        } else if (restaurantChoice == runAllScenariosChoice()) {
            runAllScenarios();
            continue;
        } else {
            const RestaurantOption& restaurant =
                kRestaurants[static_cast<std::size_t>(restaurantChoice - 1)];
            const int demandChoice = promptForDemandChoice(restaurant);
            if (demandChoice == 0) {
                continue;
            }

            const std::string demandLevel = (demandChoice == 1) ? "non_peak" : "peak";
            const ScenarioOption* scenario = findScenario(restaurant.key, demandLevel);
            if (scenario == nullptr) {
                std::cout << "Could not find a scenario for that selection.\n";
                continue;
            }

            selectedScenario = *scenario;
        }

        runScenario(selectedScenario);
    }
}
