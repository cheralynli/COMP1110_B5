#include "fcfs_simulation.h"
#include "restaurant_parser.h"
#include "simulation.h"
#include "size_queue_simulation.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
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
    std::string label;
    std::string description;
    std::string configPath;
    std::string arrivalsPath;
};

struct PairOption {
    std::string pairId;
    std::string restaurantKey;
    std::string restaurantLabel;
    std::string title;
    std::string factorChanged;
    std::string comparisonFocus;
    ScenarioOption optionA;
    ScenarioOption optionB;
};

struct CategoryOption {
    std::string key;
    std::string label;
    std::string description;
};

struct AlgorithmOption {
    AlgorithmType type;
    std::string key;
    std::string label;
    std::string description;
};

struct SimulationSettings {
    double fairnessWeight = 1.0;
    int lookAheadWindow = 15;
};

struct RunOutcome {
    bool success = false;
    SimulationSummary summary;
    std::string logPath;
};

struct AlgorithmPromptResult {
    const AlgorithmOption* algorithm = nullptr;
    bool runAllAlgorithms = false;
};

struct BatchComparisonResult {
    std::string algorithmLabel;
    std::string restaurantLabel;
    std::string pairTitle;
    std::string scenarioAName;
    std::string scenarioBName;
    RunOutcome outcomeA;
    RunOutcome outcomeB;
};

const std::vector<AlgorithmOption> kAlgorithms = {
    {
        AlgorithmType::Custom,
        "custom",
        "Our Custom Algorithm",
        "Uses utility, waiting time, dining duration, and opportunity cost to decide who to seat.",
    },
    {
        AlgorithmType::FCFS,
        "fcfs",
        "FCFS",
        "Seats the earliest arriving group that fits an available table.",
    },
    {
        AlgorithmType::SizeQueue,
        "size_queue",
        "Size-Based Queue",
        "Uses fixed 1-2, 3-4, and 5+ person queues; larger tables prefer larger groups first.",
    },
};

const std::vector<CategoryOption> kCategories = {
    {
        "fastfood",
        "Fast Food",
        "Shorter dining times, quick turnover, and small to medium groups.",
    },
    {
        "cafe",
        "Cafe",
        "Mostly small groups with a few medium groups and moderate stay lengths.",
    },
    {
        "family",
        "Family Dining",
        "Larger groups with longer dining durations and stronger table-size tradeoffs.",
    },
    {
        "sushi",
        "Sushi Belt",
        "Counter-style dining with many solo diners and pairs, plus a few small groups.",
    },
    {
        "kbbq",
        "KBBQ / Hotpot",
        "Longer dining times, slower turnover, and more medium-to-large group pressure.",
    },
    {
        "all",
        "All Restaurant Styles",
        "Browse every comparison study in one menu.",
    },
};

const std::vector<PairOption> kPairs = {
    {
        "family_dinner",
        "family",
        "Family Dining",
        "Balanced vs Large-Table-Heavy Layout",
        "table size mix",
        "Use this study to compare a balanced family dining layout against one that strongly favors larger tables.",
        {
            "family_dinner_A",
            "Variation A",
            "Balanced 2-seat, 4-seat, and 6-seat layout for mixed family groups.",
            "input/default/family_dinner_A/config.txt",
            "input/default/family_dinner_A/arrivals.txt",
        },
        {
            "family_dinner_B",
            "Variation B",
            "Large-table-heavy layout with fewer small-table options for families.",
            "input/default/family_dinner_B/config.txt",
            "input/default/family_dinner_B/arrivals.txt",
        },
    },
    {
        "cafe_lunch",
        "cafe",
        "Cafe",
        "Two-Seat Heavy vs Mixed Cafe Layout",
        "table size mix",
        "Use this study to compare a cafe dominated by 2-seat tables against a more flexible mixed layout.",
        {
            "cafe_lunch_A",
            "Variation A",
            "Mostly 2-seat cafe tables with only one 4-seat table.",
            "input/default/cafe_lunch_A/config.txt",
            "input/default/cafe_lunch_A/arrivals.txt",
        },
        {
            "cafe_lunch_B",
            "Variation B",
            "A more mixed cafe layout with more 4-seat flexibility.",
            "input/default/cafe_lunch_B/config.txt",
            "input/default/cafe_lunch_B/arrivals.txt",
        },
    },
    {
        "fastfood_rush",
        "fastfood",
        "Fast Food",
        "Limited vs Expanded Rush Capacity",
        "total seating capacity",
        "Use this study to compare whether expanding rush-hour seating lowers waits enough to justify the extra capacity.",
        {
            "fastfood_rush_A",
            "Variation A",
            "Limited seating capacity during the rush.",
            "input/default/fastfood_rush_A/config.txt",
            "input/default/fastfood_rush_A/arrivals.txt",
        },
        {
            "fastfood_rush_B",
            "Variation B",
            "Expanded seating capacity with the same arrivals and durations.",
            "input/default/fastfood_rush_B/config.txt",
            "input/default/fastfood_rush_B/arrivals.txt",
        },
    },
    {
        "sushi_belt",
        "sushi",
        "Sushi Belt",
        "Solo-Seat Priority",
        "small-seat vs group-seat emphasis",
        "Use this study to compare a sushi belt that prioritizes solo and pair seating against one that gives up solo spots for more 4-seat flexibility.",
        {
            "sushi_belt_A",
            "Variation A",
            "Many 1-seat and 2-seat spots, optimized for solo diners and pairs.",
            "input/default/sushi_belt_A/config.txt",
            "input/default/sushi_belt_A/arrivals.txt",
        },
        {
            "sushi_belt_B",
            "Variation B",
            "Fewer solo spots and more 4-seat tables for small groups.",
            "input/default/sushi_belt_B/config.txt",
            "input/default/sushi_belt_B/arrivals.txt",
        },
    },
    {
        "kbbq_hotpot",
        "kbbq",
        "KBBQ / Hotpot",
        "Medium vs Large Group Focus",
        "layout focus for expected group sizes",
        "Use this study to compare a KBBQ layout built around 4-seat tables against one with more 6-seat tables for longer large-group sessions.",
        {
            "kbbq_hotpot_A",
            "Variation A",
            "More 4-seat tables for medium groups and couples joining another pair.",
            "input/default/kbbq_hotpot_A/config.txt",
            "input/default/kbbq_hotpot_A/arrivals.txt",
        },
        {
            "kbbq_hotpot_B",
            "Variation B",
            "More 6-seat tables to better support large groups during long stays.",
            "input/default/kbbq_hotpot_B/config.txt",
            "input/default/kbbq_hotpot_B/arrivals.txt",
        },
    },
};

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

        width += (codepoint >= 0x1100U) ? 2 : 1;
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

void pauseForEnter() {
    std::cout << "\nPress Enter to continue.";
    std::string line;
    std::getline(std::cin, line);
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
        renderCenteredBlock(
            {title, cat},
            {
                "[s] Start Case Study   [q] Quit",
                "",
                "Choose an option: ",
            }
        );

        std::string line;
        std::getline(std::cin, line);
        if (line.empty()) {
            continue;
        }

        const char choice = static_cast<char>(std::tolower(static_cast<unsigned char>(line[0])));
        if (choice == 's' || choice == 'q') {
            return choice;
        }
    }
}

AlgorithmPromptResult promptForAlgorithmChoice() {
    while (true) {
        clearScreen();
        std::cout << "\nChoose an algorithm to run\n\n";
        for (std::size_t i = 0; i < kAlgorithms.size(); ++i) {
            std::cout << "  " << (i + 1) << ". "
                      << std::left << std::setw(22) << kAlgorithms[i].label
                      << " - " << kAlgorithms[i].description << '\n';
        }
        std::cout << "  " << (kAlgorithms.size() + 1) << ". "
                  << std::left << std::setw(22) << "Run Everything"
                  << " - Run A vs B for every case study using every algorithm.\n";
        std::cout << "  0. Back\n";
        std::cout << "\nWhat are you comparing here?\n";
        std::cout << "  You will keep the same arrivals inside each pair and change only one restaurant factor.\n";
        std::cout << "  This menu decides the seating logic you want to test on those same scenarios.\n";
        std::cout << "\nChoose an algorithm: ";

        int choice = -1;
        if (std::cin >> choice) {
            clearInputState();
            if (choice == 0) {
                return AlgorithmPromptResult{};
            }
            if (choice >= 1 && choice <= static_cast<int>(kAlgorithms.size())) {
                return AlgorithmPromptResult{&kAlgorithms[choice - 1], false};
            }
            if (choice == static_cast<int>(kAlgorithms.size() + 1)) {
                return AlgorithmPromptResult{nullptr, true};
            }
        } else {
            clearInputState();
        }
    }
}

int promptForCategoryChoice() {
    while (true) {
        clearScreen();
        std::cout << "\nChoose a restaurant case study\n\n";
        for (std::size_t i = 0; i < kCategories.size(); ++i) {
            std::cout << "  " << (i + 1) << ". "
                      << std::left << std::setw(24) << kCategories[i].label
                      << " - " << kCategories[i].description << '\n';
        }
        std::cout << "  " << (kCategories.size() + 1) << ". Custom file paths"
                  << " - Run one config/arrivals pair directly\n";
        std::cout << "  0. Back\n";
        std::cout << "\nWhat are you comparing after this?\n";
        std::cout << "  Each restaurant has one main A vs B setup study.\n";
        std::cout << "  A and B keep the same arrivals and change one restaurant-setup factor.\n";
        std::cout << "\nChoose a restaurant case study: ";

        int choice = -1;
        if (std::cin >> choice) {
            clearInputState();
            if (choice >= 0 && choice <= static_cast<int>(kCategories.size() + 1)) {
                return choice;
            }
        } else {
            clearInputState();
        }
    }
}

std::vector<const PairOption*> collectPairsForCategory(const std::string& categoryKey) {
    std::vector<const PairOption*> result;
    for (const PairOption& pair : kPairs) {
        if (categoryKey == "all" || pair.restaurantKey == categoryKey) {
            result.push_back(&pair);
        }
    }
    return result;
}

const PairOption* promptForPairChoice(const CategoryOption& category) {
    const std::vector<const PairOption*> pairs = collectPairsForCategory(category.key);
    if (pairs.size() == 1) {
        return pairs.front();
    }

    while (true) {
        clearScreen();
        std::cout << "\n" << category.label << " Comparison Studies\n\n";
        std::cout << "Each study below compares variation A and variation B.\n";
        std::cout << "Inside each study, the arrivals stay the same and only one restaurant factor changes.\n\n";

        for (std::size_t i = 0; i < pairs.size(); ++i) {
            const PairOption& pair = *pairs[i];
            std::cout << "  " << (i + 1) << ". " << pair.title << '\n';
            std::cout << "     Factor changed: " << pair.factorChanged << '\n';
            std::cout << "     Same demand in A and B. Only the restaurant setup changes.\n";
            std::cout << "     A: " << pair.optionA.description << '\n';
            std::cout << "     B: " << pair.optionB.description << '\n';
            std::cout << "     What this study tells you: " << pair.comparisonFocus << "\n\n";
        }

        std::cout << "  0. Back\n";
        std::cout << "\nChoose a comparison study: ";

        int choice = -1;
        if (std::cin >> choice) {
            clearInputState();
            if (choice == 0) {
                return nullptr;
            }
            if (choice >= 1 && choice <= static_cast<int>(pairs.size())) {
                return pairs[choice - 1];
            }
        } else {
            clearInputState();
        }
    }
}

int promptForPairAction(const PairOption& pair) {
    while (true) {
        clearScreen();
        std::cout << "\n" << pair.restaurantLabel << ": " << pair.title << "\n\n";
        std::cout << "Restaurant theme: " << pair.restaurantLabel << '\n';
        std::cout << "What you are comparing: which restaurant setup works better for the same demand.\n";
        std::cout << "What stays the same: the arrivals file, group sizes, and dining-duration pattern.\n";
        std::cout << "The one factor that changes: " << pair.factorChanged << '\n';
        std::cout << "The two restaurant setups are:\n";
        std::cout << "  A: " << pair.optionA.description << '\n';
        std::cout << "  B: " << pair.optionB.description << '\n';
        std::cout << "\nHow to compare this pair well:\n";
        std::cout << "  Look at average wait, max wait, groups served, utilization, service level within 15 minutes,\n";
        std::cout << "  and max queue length. Lower wait and queue are better, while higher served/utilization/service\n";
        std::cout << "  level are usually better.\n\n";
        std::cout << "  1. Run variation A only\n";
        std::cout << "  2. Run variation B only\n";
        std::cout << "  3. Compare A vs B side by side\n";
        std::cout << "  0. Back\n";
        std::cout << "\nChoose an action: ";

        int choice = -1;
        if (std::cin >> choice) {
            clearInputState();
            if (choice >= 0 && choice <= 3) {
                return choice;
            }
        } else {
            clearInputState();
        }
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

SimulationSettings promptForAlgorithmSettings(const AlgorithmOption& algorithm) {
    SimulationSettings settings;
    if (algorithm.type != AlgorithmType::Custom) {
        return settings;
    }

    clearScreen();
    std::cout << "\nCustom algorithm settings\n\n";
    std::cout << "These settings only affect the custom heuristic.\n";
    std::cout << "FCFS and size-based queue ignore them.\n\n";
    settings.fairnessWeight = promptForDouble("Fairness weight", 1.0);
    settings.lookAheadWindow = promptForInt("Look-ahead window (minutes)", 15);
    return settings;
}

ScenarioOption promptForCustomScenario() {
    ScenarioOption option;
    option.name = "custom_run";
    option.label = "Custom files";
    option.description = "Direct run using a config file path and an arrivals file path.";

    clearScreen();
    std::cout << "Custom scenario input\n\n";
    std::cout << "Optional short scenario name for logs [custom_run]: ";
    std::string line;
    std::getline(std::cin, line);
    if (!line.empty()) {
        option.name = line;
    }

    std::cout << "Enter config file path: ";
    std::getline(std::cin, option.configPath);
    std::cout << "Enter arrivals file path: ";
    std::getline(std::cin, option.arrivalsPath);
    return option;
}

std::string buildLogPath(const std::string& scenarioName, const std::string& algorithmKey) {
    if (algorithmKey == "fcfs") {
        return "output/fcfs_seating_log_" + scenarioName + ".csv";
    }

    if (algorithmKey == "size_queue") {
        return "output/size_seating_log_" + scenarioName + ".csv";
    }

    return "output/seating_log_" + scenarioName + ".csv";
}

struct SeatingLogRecord {
    int groupSize = 0;
    int arrivalTime = 0;
    int seatingTime = 0;
    int waitTime = 0;
    int diningDuration = 0;
    int departureTime = 0;
};

std::vector<std::string> splitCsvRow(const std::string& row) {
    std::vector<std::string> fields;
    std::stringstream stream(row);
    std::string field;

    while (std::getline(stream, field, ',')) {
        fields.push_back(field);
    }

    return fields;
}

std::vector<SeatingLogRecord> readSeatingLogRecords(const std::string& logPath) {
    std::vector<SeatingLogRecord> records;
    std::ifstream input(logPath);
    if (!input.is_open()) {
        return records;
    }

    std::string line;
    std::getline(input, line);

    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }

        const std::vector<std::string> fields = splitCsvRow(line);
        if (fields.size() < 9) {
            continue;
        }

        records.push_back(
            SeatingLogRecord{
                std::stoi(fields[1]),
                std::stoi(fields[2]),
                std::stoi(fields[3]),
                std::stoi(fields[4]),
                std::stoi(fields[7]),
                std::stoi(fields[8]),
            }
        );
    }

    return records;
}

int totalSeatsFromTables(const std::vector<Table>& tables) {
    int totalSeats = 0;
    for (const Table& table : tables) {
        totalSeats += table.capacity;
    }
    return totalSeats;
}

int maxQueueLengthFromLogRecords(const std::vector<SeatingLogRecord>& records) {
    std::map<int, int> arrivalsByTime;
    std::map<int, int> seatingsByTime;
    std::map<int, bool> eventTimes;

    for (const SeatingLogRecord& record : records) {
        arrivalsByTime[record.arrivalTime]++;
        seatingsByTime[record.seatingTime]++;
        eventTimes[record.arrivalTime] = true;
        eventTimes[record.seatingTime] = true;
    }

    int currentQueue = 0;
    int maxQueue = 0;
    for (const auto& event : eventTimes) {
        const int time = event.first;
        currentQueue += arrivalsByTime[time];
        maxQueue = std::max(maxQueue, currentQueue);
        currentQueue -= seatingsByTime[time];
    }

    return maxQueue;
}

SimulationSummary buildSummaryFromSeatingLog(
    const std::string& logPath,
    const std::vector<Table>& tables
) {
    const std::vector<SeatingLogRecord> records = readSeatingLogRecords(logPath);
    const int totalSeats = totalSeatsFromTables(tables);

    SimulationSummary summary;
    summary.groupsServed = static_cast<int>(records.size());
    summary.maxQueueLength = maxQueueLengthFromLogRecords(records);

    int totalWait = 0;
    int groupsWithin15 = 0;
    int usedSeatMinutes = 0;
    int endTime = 0;

    for (const SeatingLogRecord& record : records) {
        totalWait += record.waitTime;
        summary.maxWait = std::max(summary.maxWait, record.waitTime);
        if (record.waitTime <= 15) {
            groupsWithin15++;
        }

        usedSeatMinutes += record.groupSize * record.diningDuration;
        endTime = std::max(endTime, record.departureTime);
    }

    summary.totalSimulationTime = endTime;
    if (summary.groupsServed > 0) {
        summary.averageWait =
            static_cast<double>(totalWait) / static_cast<double>(summary.groupsServed);
        summary.serviceLevel15 =
            (static_cast<double>(groupsWithin15) / static_cast<double>(summary.groupsServed)) * 100.0;
    }

    if (totalSeats > 0 && endTime > 0) {
        summary.tableUtilization =
            (static_cast<double>(usedSeatMinutes) /
             static_cast<double>(totalSeats * endTime)) * 100.0;
    }

    return summary;
}

void printSummary(const SimulationSummary& summary) {
    std::cout << "\nSummary metrics\n";
    std::cout << "  Groups served:             " << summary.groupsServed << '\n';
    std::cout << "  Average wait time:         " << std::fixed << std::setprecision(2)
              << summary.averageWait << " minutes\n";
    std::cout << "  Maximum wait time:         " << summary.maxWait << " minutes\n";
    std::cout << "  Table utilization:         " << std::fixed << std::setprecision(2)
              << summary.tableUtilization << "%\n";
    std::cout << "  Service level within 15m:  " << std::fixed << std::setprecision(2)
              << summary.serviceLevel15 << "%\n";
    std::cout << "  Maximum queue length:      " << summary.maxQueueLength << '\n';
}

RunOutcome runScenario(
    const ScenarioOption& scenario,
    const AlgorithmOption& algorithm,
    const SimulationSettings& settings,
    bool pauseAfterRun
) {
    InputParser parser;
    parser.loadConfig(scenario.configPath);
    parser.loadArrivals(scenario.arrivalsPath);

    const std::vector<Table>& tables = parser.getTables();
    const std::vector<Group>& arrivals = parser.getArrivals();
    const std::vector<QueueRule>& queueRules = parser.getQueueRules();

    RunOutcome outcome;
    if (tables.empty()) {
        std::cout << "No tables were loaded from " << scenario.configPath << ".\n";
        if (pauseAfterRun) {
            pauseForEnter();
        }
        return outcome;
    }

    if (arrivals.empty()) {
        std::cout << "No arrivals were loaded from " << scenario.arrivalsPath << ".\n";
        if (pauseAfterRun) {
            pauseForEnter();
        }
        return outcome;
    }

    std::filesystem::create_directories("output");
    outcome.logPath = buildLogPath(scenario.name, algorithm.key);

    clearScreen();
    std::cout << "\nRunning " << scenario.name << " with " << algorithm.label << "\n\n";
    std::cout << "Scenario meaning: " << scenario.description << '\n';
    std::cout << "Config:   " << scenario.configPath << '\n';
    std::cout << "Arrivals: " << scenario.arrivalsPath << '\n';
    std::cout << "Log file: " << outcome.logPath << "\n\n";

    if (algorithm.type == AlgorithmType::Custom) {
        WokThisWaySim simulation(
            tables,
            queueRules,
            AlgorithmType::Custom,
            settings.fairnessWeight,
            settings.lookAheadWindow
        );
        simulation.precomputeHourlyRates(arrivals);
        simulation.setSeatingLogPath(outcome.logPath);
        outcome.summary = simulation.runSimulation(arrivals);
    } else if (algorithm.type == AlgorithmType::FCFS) {
        FCFSSimulation simulation(tables);
        simulation.setSeatingLogPath(outcome.logPath);
        simulation.runSimulation(arrivals);
        outcome.summary = buildSummaryFromSeatingLog(outcome.logPath, tables);
    } else {
        SizeQueueSimulation simulation(tables);
        simulation.setSeatingLogPath(outcome.logPath);
        simulation.runSimulation(arrivals);
        outcome.summary = buildSummaryFromSeatingLog(outcome.logPath, tables);
    }

    outcome.success = true;

    printSummary(outcome.summary);
    std::cout << "\nSeating log saved to " << outcome.logPath << ".\n";

    if (pauseAfterRun) {
        pauseForEnter();
    }

    return outcome;
}

void printComparisonRow(
    const std::string& metric,
    double valueA,
    double valueB,
    bool lowerIsBetter,
    const std::string& unit
) {
    std::string better = "Tie";
    if (valueA != valueB) {
        if (lowerIsBetter) {
            better = (valueA < valueB) ? "A" : "B";
        } else {
            better = (valueA > valueB) ? "A" : "B";
        }
    }

    std::cout << std::left << std::setw(28) << metric
              << std::right << std::setw(12) << std::fixed << std::setprecision(2) << valueA
              << std::setw(12) << std::fixed << std::setprecision(2) << valueB
              << std::setw(10) << better
              << "    " << unit << '\n';
}

void showPairComparison(
    const PairOption& pair,
    const AlgorithmOption& algorithm,
    const RunOutcome& outcomeA,
    const RunOutcome& outcomeB
) {
    clearScreen();
    std::cout << "\nDetailed comparison for " << pair.restaurantLabel << ": " << pair.title
              << " using " << algorithm.label << "\n\n";
    std::cout << "Restaurant theme: " << pair.restaurantLabel << '\n';
    std::cout << "What you are comparing: two restaurant setups under the same demand.\n";
    std::cout << "Same in A and B: arrivals, group sizes, and dining-duration pattern.\n";
    std::cout << "Changed factor: " << pair.factorChanged << '\n';
    std::cout << "A setup: " << pair.optionA.description << '\n';
    std::cout << "B setup: " << pair.optionB.description << '\n';
    std::cout << "Why this study exists: " << pair.comparisonFocus << "\n\n";

    std::cout << std::left << std::setw(28) << "Metric"
              << std::right << std::setw(12) << "A"
              << std::setw(12) << "B"
              << std::setw(10) << "Better"
              << "    Meaning\n";
    std::cout << std::string(78, '-') << '\n';
    printComparisonRow("Average wait", outcomeA.summary.averageWait, outcomeB.summary.averageWait, true, "lower is better");
    printComparisonRow("Maximum wait", static_cast<double>(outcomeA.summary.maxWait), static_cast<double>(outcomeB.summary.maxWait), true, "lower is better");
    printComparisonRow("Groups served", static_cast<double>(outcomeA.summary.groupsServed), static_cast<double>(outcomeB.summary.groupsServed), false, "higher is better");
    printComparisonRow("Table utilization %", outcomeA.summary.tableUtilization, outcomeB.summary.tableUtilization, false, "higher can be better");
    printComparisonRow("Service within 15m %", outcomeA.summary.serviceLevel15, outcomeB.summary.serviceLevel15, false, "higher is better");
    printComparisonRow("Max queue length", static_cast<double>(outcomeA.summary.maxQueueLength), static_cast<double>(outcomeB.summary.maxQueueLength), true, "lower is better");

    std::cout << "\nInterpret this carefully:\n";
    std::cout << "  If one side has lower wait but much lower utilization, it may simply be under less pressure.\n";
    std::cout << "  If one side has higher utilization and still keeps waits low, that layout is usually stronger.\n";
    std::cout << "  Because the arrivals are identical, the differences above come from the restaurant setup, not demand.\n";
    std::cout << "\nLogs saved to:\n";
    std::cout << "  A -> " << outcomeA.logPath << '\n';
    std::cout << "  B -> " << outcomeB.logPath << '\n';
    pauseForEnter();
}

std::string shortened(const std::string& value, std::size_t maxWidth) {
    if (value.size() <= maxWidth) {
        return value;
    }

    if (maxWidth <= 3) {
        return value.substr(0, maxWidth);
    }

    return value.substr(0, maxWidth - 3) + "...";
}

std::string betterVariation(double valueA, double valueB, bool lowerIsBetter) {
    if (valueA == valueB) {
        return "Tie";
    }

    if (lowerIsBetter) {
        return (valueA < valueB) ? "A" : "B";
    }

    return (valueA > valueB) ? "A" : "B";
}

void showBatchComparisonSummary(const std::vector<BatchComparisonResult>& results) {
    clearScreen();
    std::cout << "\nAll algorithms / all case studies complete\n\n";
    std::cout << "Each row compares variation A and variation B for the same case study.\n";
    std::cout << "Custom runs used the default settings: fairness weight 1.0, look-ahead window 15 minutes.\n\n";

    std::cout << std::left
              << std::setw(16) << "Algorithm"
              << std::setw(18) << "Restaurant"
              << std::setw(34) << "Case study"
              << std::right
              << std::setw(9) << "Avg A"
              << std::setw(9) << "Avg B"
              << std::setw(8) << "Wait"
              << std::setw(9) << "Util A"
              << std::setw(9) << "Util B"
              << std::setw(8) << "QueueA"
              << std::setw(8) << "QueueB"
              << '\n';
    std::cout << std::string(128, '-') << '\n';

    for (const BatchComparisonResult& result : results) {
        if (!result.outcomeA.success || !result.outcomeB.success) {
            std::cout << std::left
                      << std::setw(16) << shortened(result.algorithmLabel, 15)
                      << std::setw(18) << shortened(result.restaurantLabel, 17)
                      << std::setw(34) << shortened(result.pairTitle, 33)
                      << "failed\n";
            continue;
        }

        std::cout << std::left
                  << std::setw(16) << shortened(result.algorithmLabel, 15)
                  << std::setw(18) << shortened(result.restaurantLabel, 17)
                  << std::setw(34) << shortened(result.pairTitle, 33)
                  << std::right
                  << std::setw(9) << std::fixed << std::setprecision(2)
                  << result.outcomeA.summary.averageWait
                  << std::setw(9) << std::fixed << std::setprecision(2)
                  << result.outcomeB.summary.averageWait
                  << std::setw(8)
                  << betterVariation(
                         result.outcomeA.summary.averageWait,
                         result.outcomeB.summary.averageWait,
                         true
                     )
                  << std::setw(9) << std::fixed << std::setprecision(2)
                  << result.outcomeA.summary.tableUtilization
                  << std::setw(9) << std::fixed << std::setprecision(2)
                  << result.outcomeB.summary.tableUtilization
                  << std::setw(8) << result.outcomeA.summary.maxQueueLength
                  << std::setw(8) << result.outcomeB.summary.maxQueueLength
                  << '\n';
    }

    std::cout << "\nLogs were written to the usual algorithm-specific files in output/.\n";
    pauseForEnter();
}

void runAllAlgorithmsForAllPairs() {
    const SimulationSettings defaultSettings;
    std::vector<BatchComparisonResult> results;

    for (const AlgorithmOption& algorithm : kAlgorithms) {
        for (const PairOption& pair : kPairs) {
            clearScreen();
            std::cout << "\nRunning all simulations\n\n";
            std::cout << "Algorithm: " << algorithm.label << '\n';
            std::cout << "Case study: " << pair.restaurantLabel << " - " << pair.title << '\n';
            std::cout << "Action: Compare A vs B side by side\n\n";

            const RunOutcome outcomeA = runScenario(
                pair.optionA,
                algorithm,
                defaultSettings,
                false
            );
            const RunOutcome outcomeB = runScenario(
                pair.optionB,
                algorithm,
                defaultSettings,
                false
            );

            results.push_back(
                BatchComparisonResult{
                    algorithm.label,
                    pair.restaurantLabel,
                    pair.title,
                    pair.optionA.name,
                    pair.optionB.name,
                    outcomeA,
                    outcomeB,
                }
            );
        }
    }

    showBatchComparisonSummary(results);
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

        const AlgorithmPromptResult algorithmChoice = promptForAlgorithmChoice();
        if (algorithmChoice.runAllAlgorithms) {
            runAllAlgorithmsForAllPairs();
            continue;
        }

        const AlgorithmOption* algorithm = algorithmChoice.algorithm;
        if (algorithm == nullptr) {
            continue;
        }

        const SimulationSettings settings = promptForAlgorithmSettings(*algorithm);

        while (true) {
            const int categoryChoice = promptForCategoryChoice();
            if (categoryChoice == 0) {
                break;
            }

            if (categoryChoice == static_cast<int>(kCategories.size() + 1)) {
                const ScenarioOption customScenario = promptForCustomScenario();
                runScenario(customScenario, *algorithm, settings, true);
                continue;
            }

            const CategoryOption& category = kCategories[categoryChoice - 1];
            const PairOption* pair = promptForPairChoice(category);
            if (pair == nullptr) {
                continue;
            }

            const int action = promptForPairAction(*pair);
            if (action == 0) {
                continue;
            }

            if (action == 1) {
                runScenario(pair->optionA, *algorithm, settings, true);
                continue;
            }

            if (action == 2) {
                runScenario(pair->optionB, *algorithm, settings, true);
                continue;
            }

            const RunOutcome outcomeA = runScenario(pair->optionA, *algorithm, settings, false);
            const RunOutcome outcomeB = runScenario(pair->optionB, *algorithm, settings, false);
            if (outcomeA.success && outcomeB.success) {
                showPairComparison(*pair, *algorithm, outcomeA, outcomeB);
            } else {
                std::cout << "\nComparison could not be completed.\n";
                pauseForEnter();
            }
        }
    }
}
