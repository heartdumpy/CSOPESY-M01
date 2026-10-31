#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "config.h"
#include "ProcessManager.h"

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

// Represents one simulated CPU core.
struct CPUCore {
    int coreId = -1;
    std::shared_ptr<Process> currentProcess = nullptr;

    std::uint64_t activeTicks = 0;
    std::uint64_t idleTicks = 0;
};

class Scheduler {
public:
    Scheduler() = default;
    ~Scheduler();

    // Starts the scheduler using the loaded configuration.
    bool startScheduler(
        ProcessManager& processManager,
        const Config& config
    );

    // Stops the scheduler.
    void stopScheduler();

    // Checks whether the scheduler is running.
    bool isRunning() const;

    // Returns information about all CPU cores.
    std::vector<CPUCore> getCores() const;

    // Returns overall CPU utilization as a percentage.
    double getCpuUtilization() const;

private:
    std::atomic<bool> running{false};

    std::thread schedulerThread;

    mutable std::mutex schedulerMutex;

    ProcessManager* processManager = nullptr;
    Config schedulerConfig{};

    std::vector<CPUCore> cores;

    // Background scheduling loop.
    void schedulerLoop();
};

#endif
