#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "config.h"
#include "ProcessManager.h"

#include <atomic>
#include <deque>
#include <thread>
#include <unordered_set>

// Internal mutable CPU-core state.
struct CPUCore {
    int coreId = -1;

    std::shared_ptr<Process> currentProcess;

    std::uint64_t activeTicks = 0;
    std::uint64_t idleTicks = 0;
    std::uint64_t quantumUsed = 0;
    std::uint64_t delayRemaining = 0;
};

// Read-only copy for reporting.
struct CoreSnapshot {
    int coreId = -1;

    std::shared_ptr<const ProcessSnapshot> currentProcess;

    std::uint64_t activeTicks = 0;
    std::uint64_t idleTicks = 0;
    std::uint64_t quantumUsed = 0;
    std::uint64_t delayRemaining = 0;
};

// Consistent system information for console and file reports.
struct SystemSnapshot {
    std::uint64_t tick = 0;

    std::size_t coresUsed = 0;
    std::size_t coresAvailable = 0;

    double utilization = 0;

    std::vector<CoreSnapshot> cores;
    std::vector<ProcessSnapshot> processes;

    std::string error;
    std::string algorithm;
    bool generating = false;
    std::size_t readyCount = 0;
};

class Scheduler {
public:
    ~Scheduler();

    // Initializes continuous execution.
    // background=false allows deterministic internal testing.
    bool initialize(
        ProcessManager& manager,
        const Config& config,
        bool background = true
    );

    // Compatibility adapter.
    bool startScheduler(
        ProcessManager& manager,
        const Config& config
    );

    // Stops execution during application shutdown.
    // Do not use this for the scheduler-stop CLI command.
    void stopScheduler();

    // Controls automatic process generation only.
    bool startGeneration();
    void stopGeneration();

    bool isGenerating() const;
    bool isRunning() const;

    std::vector<CoreSnapshot> getCores() const;
    double getCpuUtilization() const;

    SystemSnapshot snapshot() const;

    // Executes one simulated CPU tick.
    void tickOnce();

private:
    std::atomic<bool> running{false};

    std::thread schedulerThread;
    mutable std::mutex schedulerMutex;

    ProcessManager* processManager = nullptr;
    Config schedulerConfig{};

    std::vector<CPUCore> cores;

    std::deque<std::shared_ptr<Process>> ready;
    std::unordered_set<int> known;

    bool generating = false;

    std::uint64_t cpuTick = 0;
    std::uint64_t nextBatchTick = 0;
    std::uint64_t nextName = 1;

    std::string engineError;

    void schedulerLoop();

    void collectReadyProcesses();
    void rotateProcesses();
    void dispatchProcess(CPUCore& core);
    void executeCore(CPUCore& core);
    void generateProcess();
    void releaseCores();
};

#endif
