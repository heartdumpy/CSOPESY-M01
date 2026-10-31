
#include "Scheduler.h"

#include <chrono>
#include <thread>

Scheduler::~Scheduler() {
    stopScheduler();
}

bool Scheduler::startScheduler(
    ProcessManager& manager,
    const Config& config
) {
    if (running.load()) {
        return false;
    }

    // Join an old thread if one is still joinable.
    if (schedulerThread.joinable()) {
        schedulerThread.join();
    }

    {
        std::lock_guard<std::mutex> lock(schedulerMutex);

        processManager = &manager;
        schedulerConfig = config;

        cores.clear();

        for (int i = 0; i < config.numCpu; ++i) {
            CPUCore core;
            core.coreId = i;
            cores.push_back(core);
        }
    }

    running = true;
    schedulerThread = std::thread(&Scheduler::schedulerLoop, this);

    return true;
}

void Scheduler::stopScheduler() {
    running = false;

    if (schedulerThread.joinable()) {
        schedulerThread.join();
    }

    std::lock_guard<std::mutex> lock(schedulerMutex);

    for (auto& core : cores) {
        if (core.currentProcess &&
            core.currentProcess->state == ProcessState::RUNNING) {
            core.currentProcess->state = ProcessState::READY;
            core.currentProcess->assignedCore = -1;
        }

        core.currentProcess.reset();
    }
}

bool Scheduler::isRunning() const {
    return running.load();
}

std::vector<CPUCore> Scheduler::getCores() const {
    std::lock_guard<std::mutex> lock(schedulerMutex);
    return cores;
}

double Scheduler::getCpuUtilization() const {
    std::lock_guard<std::mutex> lock(schedulerMutex);

    std::uint64_t active = 0;
    std::uint64_t idle = 0;

    for (const auto& core : cores) {
        active += core.activeTicks;
        idle += core.idleTicks;
    }

    const std::uint64_t total = active + idle;

    if (total == 0) {
        return 0.0;
    }

    return (static_cast<double>(active) /
            static_cast<double>(total)) * 100.0;
}

void Scheduler::schedulerLoop() {
    std::vector<std::uint64_t> quantumUsed(
        static_cast<std::size_t>(schedulerConfig.numCpu), 0
    );

    while (running.load()) {
        {
            std::lock_guard<std::mutex> lock(schedulerMutex);

            if (processManager == nullptr) {
                running = false;
                break;
            }

            auto processes = processManager->getAllProcesses();

            for (std::size_t i = 0; i < cores.size(); ++i) {
                CPUCore& core = cores[i];

                // Handle a process already assigned to this core.
                if (core.currentProcess) {
                    auto& process = core.currentProcess;

                    if (process->state == ProcessState::TERMINATED) {
                        process->assignedCore = -1;
                        core.currentProcess.reset();
                        quantumUsed[i] = 0;
                    }
                    else if (schedulerConfig.scheduler == "rr" &&
                             quantumUsed[i] >= schedulerConfig.quantumCycles) {
                        process->state = ProcessState::READY;
                        process->assignedCore = -1;
                        core.currentProcess.reset();
                        quantumUsed[i] = 0;
                    }
                }

                // Find the next READY process if this core is idle.
                if (!core.currentProcess) {
                    for (const auto& process : processes) {
                        if (process &&
                            process->state == ProcessState::READY &&
                            process->assignedCore == -1) {
                            core.currentProcess = process;
                            process->state = ProcessState::RUNNING;
                            process->assignedCore = core.coreId;
                            quantumUsed[i] = 0;
                            break;
                        }
                    }
                }

                // Execute one simulated instruction.
                if (core.currentProcess) {
                    auto& process = core.currentProcess;

                    if (process->instructionPointer <
                        process->totalInstructions) {
                        ++process->instructionPointer;
                        ++core.activeTicks;
                        ++quantumUsed[i];
                    }

                    if (process->instructionPointer >=
                        process->totalInstructions) {
                        process->state = ProcessState::TERMINATED;
                        process->assignedCore = -1;
                        core.currentProcess.reset();
                        quantumUsed[i] = 0;
                    }
                }
                else {
                    ++core.idleTicks;
                }
            }
        }

        // Treat delays-per-exec as a delay in milliseconds
        // for this initial integration implementation.
        if (schedulerConfig.delaysPerExec > 0) {
            std::this_thread::sleep_for(
                std::chrono::milliseconds(
                    schedulerConfig.delaysPerExec
                )
            );
        }
        else {
            std::this_thread::sleep_for(
                std::chrono::milliseconds(1)
            );
        }
    }
}
