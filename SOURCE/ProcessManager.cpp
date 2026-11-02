#include "ProcessManager.h"

#include <limits>

ProcessManager::ProcessManager(
    std::uint64_t seed
)
    : random(seed),
      generator(seed) {
}

std::shared_ptr<Process> ProcessManager::insert(
    const std::string& name,
    std::vector<Instruction> program
) {
    const bool invalidName =
        name.empty() ||
        name.find_first_of(" \t\r\n") !=
            std::string::npos;

    const bool exhaustedIds =
        nextPid == std::numeric_limits<int>::max();

    if (invalidName || exhaustedIds) {
        return nullptr;
    }

    for (const auto& process : processes) {
        if (process->name == name) {
            return nullptr;
        }
    }

    auto process = std::make_shared<Process>();

    process->setProgram(std::move(program));

    process->pid = nextPid++;
    process->name = name;

    process->creationTime =
        std::chrono::system_clock::now();

    processes.push_back(process);

    return process;
}

std::shared_ptr<Process> ProcessManager::createProcess(
    const std::string& name,
    std::uint64_t minInstructions,
    std::uint64_t maxInstructions
) {
    if (
        minInstructions == 0 ||
        minInstructions > maxInstructions ||
        maxInstructions > 4294967296ULL
    ) {
        return nullptr;
    }

    std::lock_guard<std::mutex> guard(mutex);

    for (const auto& process : processes) {
        if (process->name == name) {
            return nullptr;
        }
    }

    std::uniform_int_distribution<std::uint64_t>
        distribution(
            minInstructions,
            maxInstructions
        );

    const std::uint64_t instructionCount =
        distribution(random);

    return insert(
        name,
        generator.generate(
            name,
            instructionCount
        )
    );
}

std::shared_ptr<Process>
ProcessManager::createProcessWithProgram(
    const std::string& name,
    std::vector<Instruction> program
) {
    std::lock_guard<std::mutex> guard(mutex);

    return insert(
        name,
        std::move(program)
    );
}

std::shared_ptr<Process>
ProcessManager::findProcessByName(
    const std::string& name
) const {
    std::lock_guard<std::mutex> guard(mutex);

    for (const auto& process : processes) {
        if (process->name == name) {
            return process;
        }
    }

    return nullptr;
}

std::shared_ptr<Process>
ProcessManager::findProcessByPid(
    int pid
) const {
    std::lock_guard<std::mutex> guard(mutex);

    for (const auto& process : processes) {
        if (process->pid == pid) {
            return process;
        }
    }

    return nullptr;
}

std::vector<std::shared_ptr<Process>>
ProcessManager::getAllProcesses() const {
    std::lock_guard<std::mutex> guard(mutex);

    return processes;
}

std::vector<ProcessSnapshot>
ProcessManager::getSnapshots() const {
    const auto handles = getAllProcesses();

    std::vector<ProcessSnapshot> snapshots;

    for (const auto& process : handles) {
        snapshots.push_back(
            process->snapshot()
        );
    }

    return snapshots;
}

bool ProcessManager::processExists(
    const std::string& name
) const {
    return static_cast<bool>(
        findProcessByName(name)
    );
}

std::size_t ProcessManager::getProcessCount() const {
    std::lock_guard<std::mutex> guard(mutex);

    return processes.size();
}