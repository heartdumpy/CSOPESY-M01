#include "Scheduler.h"
#include "InstructionExecutor.h"

#include <chrono>
#include <iomanip>
#include <sstream>
#include <stdexcept>

Scheduler::~Scheduler() {
    stopScheduler();
}

bool Scheduler::initialize(
    ProcessManager& manager,
    const Config& config,
    bool background
) {
    if (running.load()) {
        return false;
    }

    const bool invalidConfiguration =
        config.numCpu < 1 ||
        config.numCpu > 128 ||
        (
            config.scheduler != "fcfs" &&
            config.scheduler != "rr"
        ) ||
        config.quantumCycles == 0 ||
        config.batchProcessFreq == 0 ||
        config.minIns == 0 ||
        config.minIns > config.maxIns ||
        config.maxIns > 4294967296ULL ||
        config.quantumCycles > 4294967296ULL ||
        config.batchProcessFreq > 4294967296ULL ||
        config.delaysPerExec > 4294967296ULL;

    if (invalidConfiguration) {
        return false;
    }

    if (schedulerThread.joinable()) {
        schedulerThread.join();
    }

    {
        std::lock_guard<std::mutex> guard(schedulerMutex);

        processManager = &manager;
        schedulerConfig = config;

        cores.clear();
        ready.clear();
        known.clear();

        cpuTick = 0;
        nextName = 1;
        generating = false;

        engineError.clear();

        for (int id = 0; id < config.numCpu; ++id) {
            CPUCore core;
            core.coreId = id;

            cores.push_back(core);
        }
    }

    running = true;

    if (background) {
        try {
            schedulerThread = std::thread(
                &Scheduler::schedulerLoop,
                this
            );
        }
        catch (...) {
            running = false;
            throw;
        }
    }

    return true;
}

bool Scheduler::startScheduler(
    ProcessManager& manager,
    const Config& config
) {
    if (
        !running.load() &&
        !initialize(manager, config)
    ) {
        return false;
    }

    return startGeneration();
}

void Scheduler::stopScheduler() {
    running = false;

    if (schedulerThread.joinable()) {
        schedulerThread.join();
    }

    std::lock_guard<std::mutex> guard(schedulerMutex);

    generating = false;

    for (CPUCore& core : cores) {
        if (!core.currentProcess) {
            continue;
        }

        std::lock_guard<std::mutex> processGuard(
            core.currentProcess->mutex
        );

        if (
            core.currentProcess->state ==
            ProcessState::RUNNING
        ) {
            core.currentProcess->state =
                ProcessState::READY;
        }

        core.currentProcess->assignedCore = -1;
        core.currentProcess.reset();
    }
}

bool Scheduler::startGeneration() {
    std::lock_guard<std::mutex> guard(schedulerMutex);

    if (!running.load() || generating) {
        return false;
    }

    generating = true;

    nextBatchTick =
        cpuTick + schedulerConfig.batchProcessFreq;

    return true;
}

void Scheduler::stopGeneration() {
    std::lock_guard<std::mutex> guard(schedulerMutex);

    generating = false;
}

bool Scheduler::isGenerating() const {
    std::lock_guard<std::mutex> guard(schedulerMutex);

    return generating;
}

bool Scheduler::isRunning() const {
    return running.load();
}

SystemSnapshot Scheduler::snapshot() const {
    std::lock_guard<std::mutex> guard(schedulerMutex);

    SystemSnapshot result;

    result.tick = cpuTick;
    result.error = engineError;

    // This lock prevents scheduler execution transitions while
    // collecting the complete report.
    if (processManager != nullptr) {
        result.processes =
            processManager->getSnapshots();
    }

    for (const CPUCore& core : cores) {
        CoreSnapshot coreSnapshot;

        if (core.currentProcess) {
            ++result.coresUsed;
        }

        coreSnapshot.coreId = core.coreId;
        coreSnapshot.activeTicks = core.activeTicks;
        coreSnapshot.idleTicks = core.idleTicks;

        if (core.currentProcess) {
            coreSnapshot.currentProcess =
                std::make_shared<const ProcessSnapshot>(
                    core.currentProcess->snapshot()
                );
        }

        result.cores.push_back(
            std::move(coreSnapshot)
        );
    }

    result.coresAvailable =
        cores.size() - result.coresUsed;

    if (cores.empty()) {
        result.utilization = 0;
    }
    else {
        result.utilization =
            100.0 *
            result.coresUsed /
            cores.size();
    }

    return result;
}

std::vector<CoreSnapshot> Scheduler::getCores() const {
    return snapshot().cores;
}

double Scheduler::getCpuUtilization() const {
    return snapshot().utilization;
}

void Scheduler::tickOnce() {
    std::lock_guard<std::mutex> guard(schedulerMutex);

    if (
        !running.load() ||
        processManager == nullptr
    ) {
        return;
    }

    ++cpuTick;

    // Discover newly registered processes and wake due sleepers.
    for (
        const auto& process :
        processManager->getAllProcesses()
    ) {
        std::lock_guard<std::mutex> processGuard(
            process->mutex
        );

        const bool newlyDiscovered =
            known.insert(process->pid).second;

        if (
            process->state == ProcessState::WAITING &&
            process->wakeUpTick <= cpuTick
        ) {
            process->state = ProcessState::READY;
            ready.push_back(process);
        }
        else if (
            newlyDiscovered &&
            process->state == ProcessState::READY
        ) {
            ready.push_back(process);
        }
    }

    // Requeue all expired RR quanta before dispatching any core.
    // This prevents one process from executing on multiple cores
    // during the same simulated CPU tick.
    for (CPUCore& core : cores) {
        const bool quantumExpired =
            core.currentProcess &&
            schedulerConfig.scheduler == "rr" &&
            core.quantumUsed >=
                schedulerConfig.quantumCycles;

        if (!quantumExpired) {
            continue;
        }

        auto process = core.currentProcess;

        {
            std::lock_guard<std::mutex> processGuard(
                process->mutex
            );

            process->state = ProcessState::READY;
            process->assignedCore = -1;
        }

        ready.push_back(process);

        core.currentProcess.reset();
        core.quantumUsed = 0;
    }

    for (CPUCore& core : cores) {
        // Assign the next ready process to an idle core.
        if (
            !core.currentProcess &&
            !ready.empty()
        ) {
            core.currentProcess = ready.front();
            ready.pop_front();

            core.quantumUsed = 0;

            std::lock_guard<std::mutex> processGuard(
                core.currentProcess->mutex
            );

            if (
                !core.currentProcess->
                    executionDelayInitialized
            ) {
                core.currentProcess->
                    executionDelayRemaining =
                        schedulerConfig.delaysPerExec;

                core.currentProcess->
                    executionDelayInitialized = true;
            }

            core.delayRemaining =
                core.currentProcess->
                    executionDelayRemaining;

            core.currentProcess->state =
                ProcessState::RUNNING;

            core.currentProcess->assignedCore =
                core.coreId;
        }

        if (!core.currentProcess) {
            ++core.idleTicks;
            continue;
        }

        ++core.activeTicks;
        ++core.quantumUsed;

        StepResult result = StepResult::CONTINUE;

        if (core.delayRemaining > 0) {
            // Simulated busy waiting: the process retains its core.
            --core.delayRemaining;
        }
        else {
            // The executor updates instruction progress itself.
            result = InstructionExecutor::executeStep(
                *core.currentProcess,
                cpuTick,
                core.coreId
            );

            core.delayRemaining =
                schedulerConfig.delaysPerExec;
        }

        {
            std::lock_guard<std::mutex> processGuard(
                core.currentProcess->mutex
            );

            // Save delay progress so short RR quanta do not
            // repeatedly restart an unfinished instruction delay.
            core.currentProcess->
                executionDelayRemaining =
                    core.delayRemaining;
        }

        if (result != StepResult::CONTINUE) {
            // SLEEPING and FINISHED both release the CPU core.
            core.currentProcess.reset();
            core.quantumUsed = 0;
        }
    }

    // Generate at the end of the CPU cycle.
    if (
        generating &&
        cpuTick >= nextBatchTick
    ) {
        std::string processName;

        do {
            std::ostringstream nameStream;

            nameStream
                << 'p'
                << std::setw(2)
                << std::setfill('0')
                << nextName++;

            processName = nameStream.str();
        }
        while (
            processManager->processExists(
                processName
            )
        );

        processManager->createProcess(
            processName,
            schedulerConfig.minIns,
            schedulerConfig.maxIns
        );

        nextBatchTick =
            cpuTick +
            schedulerConfig.batchProcessFreq;
    }
}

void Scheduler::schedulerLoop() {
    try {
        while (running.load()) {
            tickOnce();

            // Host pacing only. Instruction delay, sleep,
            // quantum and generation use simulated CPU ticks.
            std::this_thread::sleep_for(
                std::chrono::milliseconds(1)
            );
        }
    }
    catch (const std::exception& exception) {
        std::lock_guard<std::mutex> guard(
            schedulerMutex
        );

        engineError = exception.what();
        running = false;
    }
}