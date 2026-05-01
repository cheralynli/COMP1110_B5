#ifndef DEFAULT_SCENARIO_LOADER_H
#define DEFAULT_SCENARIO_LOADER_H

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <string>
#include <vector>

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

inline std::string formatScenarioMenuLabel(const std::string& raw) {
    if (raw == "fastfood") {
        return "Fast Food";
    }

    std::string label = raw;
    for (char& ch : label) {
        if (ch == '_' || ch == '-') {
            ch = ' ';
        }
    }

    bool nextUpper = true;
    for (char& ch : label) {
        unsigned char value = static_cast<unsigned char>(ch);

        if (ch == ' ') {
            nextUpper = true;
        } else if (nextUpper) {
            ch = static_cast<char>(std::toupper(value));
            nextUpper = false;
        } else {
            ch = static_cast<char>(std::tolower(value));
        }
    }

    return label;
}

inline std::vector<ScenarioOption> loadDefaultScenarios(
    const std::filesystem::path& root = std::filesystem::path("input") / "default"
) {
    std::vector<ScenarioOption> scenarios;

    if (!std::filesystem::exists(root)) {
        return scenarios;
    }

    if (!std::filesystem::is_directory(root)) {
        return scenarios;
    }

    std::vector<std::string> folderNames;
    for (const auto& entry : std::filesystem::directory_iterator(root)) {
        if (entry.is_directory()) {
            folderNames.push_back(entry.path().filename().string());
        }
    }

    std::sort(folderNames.begin(), folderNames.end());

    for (const std::string& folderName : folderNames) {
        std::filesystem::path folderPath = root / folderName;
        std::filesystem::path configPath = folderPath / "config.txt";
        std::filesystem::path arrivalsPath = folderPath / "arrivals.txt";

        if (!std::filesystem::exists(configPath) || !std::filesystem::exists(arrivalsPath)) {
            continue;
        }

        std::size_t underscorePos = folderName.find('_');
        if (underscorePos == std::string::npos) {
            continue;
        }

        std::string restaurantName = folderName.substr(0, underscorePos);
        std::string scenarioName = folderName.substr(underscorePos + 1);

        if (restaurantName.empty() || scenarioName.empty()) {
            continue;
        }

        ScenarioOption scenario;
        scenario.name = folderName;
        scenario.restaurantType = restaurantName;
        scenario.demandLevel = scenarioName;
        scenario.description = formatScenarioMenuLabel(scenarioName);
        scenario.configPath = configPath.string();
        scenario.arrivalsPath = arrivalsPath.string();
        scenarios.push_back(scenario);
    }

    return scenarios;
}

inline std::vector<RestaurantOption> buildRestaurantOptions(
    const std::vector<ScenarioOption>& scenarios
) {
    std::vector<RestaurantOption> restaurants;

    for (const ScenarioOption& scenario : scenarios) {
        bool found = false;
        for (const RestaurantOption& restaurant : restaurants) {
            if (restaurant.key == scenario.restaurantType) {
                found = true;
                break;
            }
        }

        if (found) {
            continue;
        }

        RestaurantOption restaurant;
        restaurant.key = scenario.restaurantType;
        restaurant.label = formatScenarioMenuLabel(scenario.restaurantType);

        for (const ScenarioOption& otherScenario : scenarios) {
            if (otherScenario.restaurantType != scenario.restaurantType) {
                continue;
            }

            if (!restaurant.description.empty()) {
                restaurant.description += " / ";
            }
            restaurant.description += otherScenario.description;
        }

        restaurants.push_back(restaurant);
    }

    return restaurants;
}

inline std::vector<const ScenarioOption*> findScenariosForRestaurant(
    const std::vector<ScenarioOption>& scenarios,
    const std::string& restaurantType
) {
    std::vector<const ScenarioOption*> matches;

    for (const ScenarioOption& scenario : scenarios) {
        if (scenario.restaurantType == restaurantType) {
            matches.push_back(&scenario);
        }
    }

    return matches;
}

#endif
