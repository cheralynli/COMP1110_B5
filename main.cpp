#include "restaurant_parser.h"
#include "simulation.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/ioctl.h>
#include <unistd.h>
#endif
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

struct TerminalSize {
    int rows;
    int cols;
};

TerminalSize getTerminalSize() {
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &consoleInfo)) {
        const int cols = consoleInfo.srWindow.Right - consoleInfo.srWindow.Left + 1;
        const int rows = consoleInfo.srWindow.Bottom - consoleInfo.srWindow.Top + 1;
        if (rows > 0 && cols > 0) {
            return {rows, cols};
        }
    }
#else
    winsize ws {};
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_row > 0 && ws.ws_col > 0) {
        return {static_cast<int>(ws.ws_row), static_cast<int>(ws.ws_col)};
    }
#endif

    return {24, 100};
}

void clearScreen() {
    std::cout << "\033[2J\033[H";
}

std::vector<std::string> splitLines(const std::string& block) {
    std::vector<std::string> lines;
    std::stringstream stream(block);
    std::string line;

    while (std::getline(stream, line)) {
        lines.push_back(line);
    }

    return lines;
}

int utf8DisplayWidth(const std::string& text) {
    int width = 0;

    for (std::size_t i = 0; i < text.size();) {
        const unsigned char ch = static_cast<unsigned char>(text[i]);
        unsigned int codepoint = 0;
        std::size_t advance = 1;

        if ((ch & 0x80U) == 0) {
            codepoint = ch;
        } else if ((ch & 0xE0U) == 0xC0U && i + 1 < text.size()) {
            codepoint = ((ch & 0x1FU) << 6) |
                        (static_cast<unsigned char>(text[i + 1]) & 0x3FU);
            advance = 2;
        } else if ((ch & 0xF0U) == 0xE0U && i + 2 < text.size()) {
            codepoint = ((ch & 0x0FU) << 12) |
                        ((static_cast<unsigned char>(text[i + 1]) & 0x3FU) << 6) |
                        (static_cast<unsigned char>(text[i + 2]) & 0x3FU);
            advance = 3;
        } else if ((ch & 0xF8U) == 0xF0U && i + 3 < text.size()) {
            codepoint = ((ch & 0x07U) << 18) |
                        ((static_cast<unsigned char>(text[i + 1]) & 0x3FU) << 12) |
                        ((static_cast<unsigned char>(text[i + 2]) & 0x3FU) << 6) |
                        (static_cast<unsigned char>(text[i + 3]) & 0x3FU);
            advance = 4;
        } else {
            codepoint = ch;
        }

        if (codepoint >= 0x1100U) {
            width += 2;
        } else {
            width += 1;
        }

        i += advance;
    }

    return width;
}

void printCenteredLine(const std::string& line, int terminalCols) {
    const int padding = std::max(0, (terminalCols - utf8DisplayWidth(line)) / 2);
    std::cout << std::string(static_cast<std::size_t>(padding), ' ') << line << '\n';
}

void printBlockCenteredAsUnit(const std::vector<std::string>& lines, int terminalCols) {
    int blockWidth = 0;
    for (const std::string& line : lines) {
        blockWidth = std::max(blockWidth, utf8DisplayWidth(line));
    }

    const int leftPadding = std::max(0, (terminalCols - blockWidth) / 2);
    const std::string padding(static_cast<std::size_t>(leftPadding), ' ');

    for (const std::string& line : lines) {
        std::cout << padding << line << '\n';
    }
}

void renderCenteredBlock(
    const std::vector<std::vector<std::string>>& blocks,
    const std::vector<std::string>& footerLines = {}
) {
    clearScreen();

    const TerminalSize terminal = getTerminalSize();
    int totalLines = static_cast<int>(footerLines.size());
    for (const std::vector<std::string>& block : blocks) {
        totalLines += static_cast<int>(block.size());
    }
    if (!blocks.empty()) {
        totalLines += static_cast<int>(blocks.size() - 1);
    }
    if (!footerLines.empty()) {
        totalLines += 1;
    }

    const int topPadding = std::max(0, (terminal.rows - totalLines) / 2);
    for (int i = 0; i < topPadding; ++i) {
        std::cout << '\n';
    }

    for (std::size_t i = 0; i < blocks.size(); ++i) {
        printBlockCenteredAsUnit(blocks[i], terminal.cols);
        if (i + 1 < blocks.size()) {
            std::cout << '\n';
        }
    }

    if (!footerLines.empty()) {
        std::cout << '\n';
        for (const std::string& line : footerLines) {
            printCenteredLine(line, terminal.cols);
        }
    }
}

void clearInputState() {
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

char promptForStartAction() {
    const std::vector<std::string> title = splitLines(
R"( _   _  __  _  _______ _  _ _   __  _   _   __ __   __
| | | |/__\| |/ /_   _| || | |/' _/| | | | /  \\ `v' /
| 'V' | \/ |   <  | | | >< | |`._`.| 'V' || /\ |`. .' 
!_/ \_!\__/|_|\_\ |_| |_||_|_||___/!_/ \_!|_||_| !_!  
)"
    );

    const std::vector<std::string> cat = splitLines(
R"(    (\
     \ \
 __    \/ ___,.-------..__        __
//\\ _,-'\\               `'--._ //\\
\\ ;'      \\                   `: //
 `(          \\                   )'
   :.          \\,----,         ,;
    `.`--.___   (    /  ___.--','
      `.     ``-----'-''     ,'
        -.                ,-
           ` -._______.-'
)"
    );

    while (true) {
        renderCenteredBlock({title, cat}, {"[s] Start   [q] Quit", "", "Choose an option: "});

        std::string line;
        std::getline(std::cin, line);
        if (line.empty()) {
            continue;
        }

        const char choice = static_cast<char>(std::tolower(static_cast<unsigned char>(line[0])));
        if (choice == 's' || choice == 'q') {
            return choice;
        }

        std::cout << "Invalid choice. Press Enter to continue.";
        std::getline(std::cin, line);
    }
}

int promptForRestaurantChoice() {
    while (true) {
        clearScreen();
        std::cout << "\nChoose a restaurant type\n";
        for (std::size_t i = 0; i < kRestaurants.size(); ++i) {
            std::cout << "  " << (i + 1) << ". "
                      << std::left << std::setw(14) << kRestaurants[i].label
                      << " - " << kRestaurants[i].description << '\n';
        }
        std::cout << "  " << customScenarioChoice() << ". Custom file paths\n";
        std::cout << "  " << runAllScenariosChoice() << ". Run all built-in scenarios\n";
        std::cout << "  0. Exit\n";
        std::cout << "\nChoose a restaurant: ";

        int choice = -1;
        if (std::cin >> choice) {
            clearInputState();
            if (choice >= 0 && choice <= runAllScenariosChoice()) {
                return choice;
            }
        } else {
            clearInputState();
        }

        std::cout << "Invalid choice. Please enter a number from the menu.\n";
    }
}

int promptForDemandChoice(const RestaurantOption& restaurant) {
    while (true) {
        clearScreen();
        std::cout << "\nChoose demand level for " << restaurant.label << '\n';
        std::cout << "  1. Non-peak\n";
        std::cout << "  2. Peak\n";
        std::cout << "  0. Back\n";
        std::cout << "\nChoose demand level: ";

        int choice = -1;
        if (std::cin >> choice) {
            clearInputState();
            if (choice >= 0 && choice <= 2) {
                return choice;
            }
        } else {
            clearInputState();
        }

        std::cout << "Invalid choice. Please enter 0, 1, or 2.\n";
    }
}

double promptForDouble(const std::string& label, double defaultValue) {
    while (true) {
        std::cout << label << " [" << defaultValue << "]: ";

        std::string line;
        std::getline(std::cin, line);
        if (line.empty()) {
            return defaultValue;
        }

        try {
            return std::stod(line);
        } catch (...) {
            std::cout << "Please enter a valid number.\n";
        }
    }
}

int promptForInt(const std::string& label, int defaultValue) {
    while (true) {
        std::cout << label << " [" << defaultValue << "]: ";

        std::string line;
        std::getline(std::cin, line);
        if (line.empty()) {
            return defaultValue;
        }

        try {
            return std::stoi(line);
        } catch (...) {
            std::cout << "Please enter a whole number.\n";
        }
    }
}

ScenarioOption promptForCustomScenario() {
    ScenarioOption option;
    option.name = "custom_run";
    option.description = "Custom input paths";

    clearScreen();
    std::cout << "Custom scenario input\n\n";
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
    return "output/seating_log_" + scenarioName + ".csv";
}

bool runScenarioWithSettings(
    const ScenarioOption& scenario,
    double fairnessWeight,
    int lookAheadWindow,
    bool clearBeforeRun,
    bool pauseAfterRun
);

bool runScenario(const ScenarioOption& scenario) {
    const double fairnessWeight = promptForDouble("Fairness weight", 1.0);
    const int lookAheadWindow = promptForInt("Look-ahead window (minutes)", 15);

    return runScenarioWithSettings(scenario, fairnessWeight, lookAheadWindow, true, true);
}

void runAllScenarios() {
    const double fairnessWeight = promptForDouble("Fairness weight", 1.0);
    const int lookAheadWindow = promptForInt("Look-ahead window (minutes)", 15);

    clearScreen();
    std::cout << "\nRunning all built-in heuristic scenarios\n";
    std::cout << "Fairness weight: " << fairnessWeight << '\n';
    std::cout << "Look-ahead window: " << lookAheadWindow << " minutes\n\n";

    int successCount = 0;
    for (const ScenarioOption& scenario : kScenarios) {
        if (runScenarioWithSettings(scenario, fairnessWeight, lookAheadWindow, false, false)) {
            successCount++;
        }
        std::cout << '\n';
    }

    std::cout << "Completed " << successCount << " of "
              << static_cast<int>(kScenarios.size()) << " built-in scenarios.\n";
    std::cout << "Press Enter to return to the main menu.";

    std::string line;
    std::getline(std::cin, line);
}

bool runScenarioWithSettings(
    const ScenarioOption& scenario,
    double fairnessWeight,
    int lookAheadWindow,
    bool clearBeforeRun,
    bool pauseAfterRun
) {
    InputParser parser;
    parser.loadConfig(scenario.configPath);
    parser.loadArrivals(scenario.arrivalsPath);

    const std::vector<Table>& tables = parser.getTables();
    const std::vector<Group>& arrivals = parser.getArrivals();

    if (tables.empty()) {
        std::cout << "No tables were loaded from " << scenario.configPath << ".\n";
        return false;
    }

    if (arrivals.empty()) {
        std::cout << "No arrivals were loaded from " << scenario.arrivalsPath << ".\n";
        return false;
    }

    WokThisWaySim simulation(tables, fairnessWeight, lookAheadWindow);
    const std::string logPath = buildLogPath(scenario.name);
    std::filesystem::create_directories("output");
    simulation.setSeatingLogPath(logPath);
    simulation.precomputeHourlyRates(arrivals);

    if (clearBeforeRun) {
        clearScreen();
    }

    std::cout << "\nRunning " << scenario.name << '\n';
    std::cout << "Config:   " << scenario.configPath << '\n';
    std::cout << "Arrivals: " << scenario.arrivalsPath << '\n';
    std::cout << "Log file: " << logPath << "\n\n";

    simulation.runSimulation(arrivals);

    std::cout << "\nSimulation complete. Seating log saved to " << logPath << ".\n";
    if (pauseAfterRun) {
        std::cout << "Press Enter to return to the main menu.";

        std::string line;
        std::getline(std::cin, line);
    }
    return true;
}

} // namespace

int main() {
    while (true) {
        const char startAction = promptForStartAction();
        if (startAction == 'q') {
            clearScreen();
            std::cout << "Exiting.\n";
            return 0;
        }

        const int restaurantChoice = promptForRestaurantChoice();
        if (restaurantChoice == 0) {
            continue;
        }

        ScenarioOption selectedScenario;
        if (restaurantChoice == customScenarioChoice()) {
            selectedScenario = promptForCustomScenario();
        } else if (restaurantChoice == runAllScenariosChoice()) {
            runAllScenarios();
            continue;
        } else {
            const RestaurantOption& restaurant = kRestaurants[restaurantChoice - 1];
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
