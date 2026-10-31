#ifndef PROCESS_MANAGER_H
#define PROCESS_MANAGER_H

#include "Process.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

class ProcessManager {
public:
    // Creates a new process with a randomly generated instruction count.
    std::shared_ptr<Process> createProcess(
        const std::string& processName,
        std::uint64_t minInstructions,
        std::uint64_t maxInstructions
    );

    // Finds a process using its name.
    std::shared_ptr<Process> findProcessByName(
        const std::string& processName
    ) const;

    // Finds a process using its PID.
    std::shared_ptr<Process> findProcessByPid(int pid) const;

    // Returns all registered processes.
    std::vector<std::shared_ptr<Process>> getAllProcesses() const;

    // Checks whether a process name already exists.
    bool processExists(const std::string& processName) const;

    // Returns the total number of registered processes.
    std::size_t getProcessCount() const;

private:
    // Stores all processes.
    std::vector<std::shared_ptr<Process>> processes;

    // Assigns unique process IDs.
    int nextPid = 1;
};

#endif
