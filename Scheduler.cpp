#include "Scheduler.h"
#include "InstructionExecutor.h"

#include <chrono>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <limits>
#include <unordered_map>

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

    releaseCores();
}

void Scheduler::releaseCores() {
    for (CPUCore& core : cores) {
        if (!core.currentProcess) {
            continue;
        }

        auto process = core.currentProcess;

        {
            std::lock_guard<std::mutex> processGuard(
                process->mutex
            );

            if (process->state == ProcessState::RUNNING) {
                process->state = ProcessState::READY;
            }

            process->assignedCore = -1;
        }

        core.currentProcess.reset();
        core.quantumUsed = 0;
        core.delayRemaining = 0;
    }
}

bool Scheduler::startGeneration() {
    std::lock_guard<std::mutex> guard(schedulerMutex);

    if (!running.load() || generating) {
        return false;
    }

    if (cpuTick > std::numeric_limits<std::uint64_t>::max() -
        schedulerConfig.batchProcessFreq) {
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
    result.algorithm = schedulerConfig.scheduler;
    result.generating = generating;
    result.readyCount = ready.size();

    // This lock prevents scheduler execution transitions while
    // collecting the complete report.
    if (processManager != nullptr) {
        result.processes =
            processManager->getSnapshots();
    }

    std::unordered_map<int, ProcessSnapshot> processCopies;

    for (const ProcessSnapshot& process : result.processes) {
        processCopies.emplace(process.pid, process);
    }

    for (const CPUCore& core : cores) {
        CoreSnapshot coreSnapshot;

        if (core.currentProcess) {
            ++result.coresUsed;
        }

        coreSnapshot.coreId = core.coreId;
        coreSnapshot.activeTicks = core.activeTicks;
        coreSnapshot.idleTicks = core.idleTicks;
        coreSnapshot.quantumUsed = core.quantumUsed;
        coreSnapshot.delayRemaining = core.delayRemaining;

        if (core.currentProcess) {
            coreSnapshot.currentProcess =
                std::make_shared<const ProcessSnapshot>(
                    processCopies.at(core.currentProcess->pid)
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

    if (cpuTick == std::numeric_limits<std::uint64_t>::max()) {
        throw std::overflow_error("CPU tick limit reached");
    }

    ++cpuTick;

    collectReadyProcesses();
    rotateProcesses();

    for (CPUCore& core : cores) {
        dispatchProcess(core);
        executeCore(core);
    }

    if (generating && cpuTick >= nextBatchTick) {
        generateProcess();
    }
}

void Scheduler::collectReadyProcesses() {
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

}

void Scheduler::rotateProcesses() {
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

}

void Scheduler::dispatchProcess(CPUCore& core) {
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

}

void Scheduler::executeCore(CPUCore& core) {
    if (!core.currentProcess) {
        ++core.idleTicks;
        return;
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

void Scheduler::generateProcess() {
    // Generate at the end of the CPU cycle.
    for (;;) {
        if (nextName == std::numeric_limits<std::uint64_t>::max()) {
            throw std::overflow_error("Automatic process name limit reached");
        }

        std::ostringstream nameStream;

        nameStream
            << 'p'
            << std::setw(2)
            << std::setfill('0')
            << nextName++;

        const std::string processName = nameStream.str();

        if (processManager->processExists(processName)) {
            continue;
        }

        auto process = processManager->createProcess(
            processName,
            schedulerConfig.minIns,
            schedulerConfig.maxIns
        );

        if (process) {
            break;
        }

        // A name may be claimed by a console command.
        if (!processManager->processExists(processName)) {
            throw std::runtime_error("Automatic process creation failed");
        }
    }

    if (cpuTick > std::numeric_limits<std::uint64_t>::max() -
        schedulerConfig.batchProcessFreq) {
        generating = false;
        engineError = "Automatic generation tick limit reached";
        return;
    }

    nextBatchTick = cpuTick + schedulerConfig.batchProcessFreq;
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
        generating = false;
        releaseCores();
    }
}