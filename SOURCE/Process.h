#ifndef PROCESS_H
#define PROCESS_H

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>

// Possible states of a process.
enum class ProcessState {
    NEW,
    READY,
    RUNNING,
    WAITING,
    TERMINATED
};

// Shared representation of a process.
struct Process {
    // Process identification.
    int pid = -1;
    std::string name;

    // Process creation information.
    std::chrono::system_clock::time_point creationTime;

    // Current execution state.
    ProcessState state = ProcessState::NEW;
    int assignedCore = -1;

    // Instructions and execution progress.
    std::vector<std::string> instructions;
    std::size_t instructionPointer = 0;
    std::size_t totalInstructions = 0;

    // Process variables.
    std::unordered_map<std::string, int> variables;

    // Execution information.
    std::vector<std::string> logs;
    std::uint64_t wakeUpTick = 0;

    // Tracks nested loop execution.
    std::vector<std::size_t> loopStack;
};

#endif
