#include "default_scenario_loader.h"
#include "fcfs_simulation.h"
#include "restaurant_parser.h"

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace {

const std::vector<ScenarioOption> kScenarios = loadDefaultScenarios();
const std::vector<RestaurantOption> kRestaurants = buildRestaurantOptions(kScenarios);

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
    std::cout << "\nFCFS baseline simulation\n";
    for (std::size_t i = 0; i < kRestaurants.size(); ++i) {
        std::cout << (i + 1) << ". " << kRestaurants[i].label
                  << " - " << kRestaurants[i].description << '\n';
    }
    std::cout << customScenarioChoice() << ". Custom file paths\n";
    std::cout << runAllScenariosChoice() << ". Run all built-in scenarios\n";
    std::cout << "0. Exit\n\n";
    return promptForChoice("Choose a scenario group: ", 0, runAllScenariosChoice());
}

int promptForScenarioChoice(
    const RestaurantOption& restaurant,
    const std::vector<const ScenarioOption*>& scenarios
) {
    std::cout << "\nChoose scenario for " << restaurant.label << '\n';
    for (std::size_t i = 0; i < scenarios.size(); ++i) {
        std::cout << (i + 1) << ". " << scenarios[i]->description << '\n';
    }
    std::cout << "0. Back\n\n";
    return promptForChoice(
        "Choose a scenario: ",
        0,
        static_cast<int>(scenarios.size())
    );
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

std::string buildLogPath(const std::string& scenarioName) {
    return "output/fcfs_seating_log_" + scenarioName + ".csv";
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

    FCFSSimulation simulation(tables);
    const std::string logPath = buildLogPath(scenario.name);
    std::filesystem::create_directories("output");
    simulation.setSeatingLogPath(logPath);

    std::cout << "\nRunning FCFS baseline for " << scenario.name << '\n';
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
    std::cout << "\nRunning all built-in FCFS scenarios\n\n";

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
            const RestaurantOption& restaurant = kRestaurants[static_cast<std::size_t>(restaurantChoice - 1)];
            const std::vector<const ScenarioOption*> scenariosForRestaurant =
                findScenariosForRestaurant(kScenarios, restaurant.key);
            const int scenarioChoice = promptForScenarioChoice(restaurant, scenariosForRestaurant);
            if (scenarioChoice == 0) {
                continue;
            }

            selectedScenario = *scenariosForRestaurant[static_cast<std::size_t>(scenarioChoice - 1)];
        }

        runScenario(selectedScenario);
    }
}
