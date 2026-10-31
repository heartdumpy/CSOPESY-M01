#ifndef CONFIG_H
#define CONFIG_H

#include <cstdint>
#include <string>

struct Config {
    int numCpu;
    std::string scheduler;

    std::uint64_t quantumCycles;
    std::uint64_t batchProcessFreq;
    std::uint64_t minIns;
    std::uint64_t maxIns;
    std::uint64_t delaysPerExec;
};

// Loads and validates the configuration file.
bool loadConfig(
    const std::string& filename,
    Config& config,
    std::string& errorMessage
);

#endif
