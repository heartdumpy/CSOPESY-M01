#ifndef PROCESS_H
#define PROCESS_H

#include "Instruction.h"

#include <chrono>
#include <memory>
#include <mutex>
#include <unordered_map>

enum class ProcessState {
    NEW,
    READY,
    RUNNING,
    WAITING,
    TERMINATED
};

// A saved position inside the main program or a FOR body.
struct LoopFrame {
    const std::vector<Instruction>* block = nullptr;

    std::size_t index = 0;
    std::uint64_t remaining = 1;
};

// Read-only value copy used by screens and reports.
struct ProcessSnapshot {
    int pid = -1;
    std::string name;

    std::chrono::system_clock::time_point creationTime;

    ProcessState state = ProcessState::NEW;
    int assignedCore = -1;

    std::uint64_t instructionPointer = 0;
    std::uint64_t totalInstructions = 0;
    std::uint64_t wakeUpTick = 0;

    std::unordered_map<std::string, std::uint16_t> variables;
    std::vector<std::string> logs;
};

struct Process {
    int pid = -1;
    std::string name;

    std::chrono::system_clock::time_point creationTime;

    ProcessState state = ProcessState::NEW;
    int assignedCore = -1;

    std::uint64_t instructionPointer = 0;
    std::uint64_t totalInstructions = 0;
    std::uint64_t wakeUpTick = 0;

    std::unordered_map<std::string, std::uint16_t> variables;
    std::vector<std::string> logs;

    // Immutable program shared by its saved loop frames.
    std::shared_ptr<const std::vector<Instruction>> instructions;

    // Preserved across sleep, preemption, and screen detachment.
    std::vector<LoopFrame> loopStack;

    // Preserves pending instruction-delay credit across RR preemption.
    std::uint64_t executionDelayRemaining = 0;
    bool executionDelayInitialized = false;

    mutable std::mutex mutex;

    Process() = default;

    Process(const Process&) = delete;
    Process& operator=(const Process&) = delete;

    void setProgram(std::vector<Instruction> program);

    ProcessSnapshot snapshot() const;
};

#endif
