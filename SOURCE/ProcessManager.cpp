
#include "ProcessManager.h"

#include <algorithm>
#include <random>
#include <chrono>

std::shared_ptr<Process> ProcessManager::createProcess(
    const std::string& processName,
    std::uint64_t minInstructions,
    std::uint64_t maxInstructions
) {
    if (processName.empty() ||
        processExists(processName) ||
        minInstructions == 0 ||
        minInstructions > maxInstructions) {
        return nullptr;
    }

    static std::mt19937_64 generator(
        std::random_device{}()
    );

    std::uniform_int_distribution<std::uint64_t> distribution(
        minInstructions,
        maxInstructions
    );

    auto process = std::make_shared<Process>();

    process->pid = nextPid++;
    process->name = processName;
    process->creationTime = std::chrono::system_clock::now();
    process->state = ProcessState::READY;
    process->assignedCore = -1;
    process->totalInstructions =
        static_cast<std::size_t>(distribution(generator));
    process->instructionPointer = 0;

    processes.push_back(process);

    return process;
}

std::shared_ptr<Process> ProcessManager::findProcessByName(
    const std::string& processName
) const {
    for (const auto& process : processes) {
        if (process && process->name == processName) {
            return process;
        }
    }

    return nullptr;
}

std::shared_ptr<Process> ProcessManager::findProcessByPid(
    int pid
) const {
    for (const auto& process : processes) {
        if (process && process->pid == pid) {
            return process;
        }
    }

    return nullptr;
}

std::vector<std::shared_ptr<Process>>
ProcessManager::getAllProcesses() const {
    return processes;
}

bool ProcessManager::processExists(
    const std::string& processName
) const {
    return findProcessByName(processName) != nullptr;
}

std::size_t ProcessManager::getProcessCount() const {
    return processes.size();
}
