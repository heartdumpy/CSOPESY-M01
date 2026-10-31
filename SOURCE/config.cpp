
#include "config.h"

#include <fstream>
#include <sstream>
#include <unordered_map>
#include <limits>

bool loadConfig(
    const std::string& filename,
    Config& config,
    std::string& errorMessage
) {
    std::ifstream file(filename);

    if (!file.is_open()) {
        errorMessage = "Unable to open configuration file: " + filename;
        return false;
    }

    std::unordered_map<std::string, std::string> settings;
    std::string line;

    while (std::getline(file, line)) {
        std::istringstream iss(line);

        std::string key;
        std::string value;

        if (!(iss >> key)) {
            continue;
        }

        // Read the remaining value, including quoted values.
        std::getline(iss >> std::ws, value);

        // Remove surrounding quotation marks.
        if (value.size() >= 2 &&
            value.front() == '"' &&
            value.back() == '"') {
            value = value.substr(1, value.size() - 2);
        }

        settings[key] = value;
    }

    const std::string requiredSettings[] = {
        "num-cpu",
        "scheduler",
        "quantum-cycles",
        "batch-process-freq",
        "min-ins",
        "max-ins",
        "delays-per-exec"
    };

    for (const std::string& key : requiredSettings) {
        if (settings.find(key) == settings.end()) {
            errorMessage = "Missing configuration setting: " + key;
            return false;
        }
    }

    try {
        unsigned long long numCpu =
            std::stoull(settings["num-cpu"]);

        if (numCpu < 1 || numCpu > 128) {
            errorMessage = "num-cpu must be between 1 and 128.";
            return false;
        }

        config.numCpu = static_cast<int>(numCpu);

        config.scheduler = settings["scheduler"];

        if (config.scheduler != "fcfs" &&
            config.scheduler != "rr") {
            errorMessage = "scheduler must be either fcfs or rr.";
            return false;
        }

        const unsigned long long MAX_VALUE = 4294967296ULL;

        auto parseNumber = [&](const std::string& key,
                               std::uint64_t& destination,
                               bool allowZero) -> bool {
            const std::string& value = settings[key];

            if (value.empty() || value.front() == '-') {
                errorMessage = "Invalid numeric value for " + key;
                return false;
            }

            std::size_t position = 0;
            unsigned long long number = std::stoull(value, &position);

            if (position != value.size()) {
                errorMessage = "Invalid numeric value for " + key;
                return false;
            }

            if ((!allowZero && number < 1) ||
                number > MAX_VALUE) {
                errorMessage = "Value out of range for " + key;
                return false;
            }

            destination = number;
            return true;
        };

        if (!parseNumber("quantum-cycles", config.quantumCycles, false))
            return false;

        if (!parseNumber("batch-process-freq", config.batchProcessFreq, false))
            return false;

        if (!parseNumber("min-ins", config.minIns, false))
            return false;

        if (!parseNumber("max-ins", config.maxIns, false))
            return false;

        if (!parseNumber("delays-per-exec", config.delaysPerExec, true))
            return false;

        if (config.minIns > config.maxIns) {
            errorMessage = "min-ins cannot be greater than max-ins.";
            return false;
        }

    } catch (const std::exception&) {
        errorMessage = "Invalid numeric value in configuration file.";
        return false;
    }

    errorMessage.clear();
    return true;
}
