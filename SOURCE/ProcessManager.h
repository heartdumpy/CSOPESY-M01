#ifndef PROCESS_MANAGER_H
#define PROCESS_MANAGER_H

#include "Process.h"
#include "InstructionGenerator.h"

class ProcessManager {
public:
    explicit ProcessManager(
        std::uint64_t seed = std::random_device{}()
    );

    std::shared_ptr<Process> createProcess(
        const std::string& name,
        std::uint64_t minInstructions,
        std::uint64_t maxInstructions
    );

    // Supports predetermined internal programs and test fixtures.
    // This is not an additional CLI command.
    std::shared_ptr<Process> createProcessWithProgram(
        const std::string& name,
        std::vector<Instruction> program
    );

    std::shared_ptr<Process> findProcessByName(
        const std::string& name
    ) const;

    std::shared_ptr<Process> findProcessByPid(
        int pid
    ) const;

    // Returns live handles for scheduler ownership.
    // Lock Process::mutex before accessing mutable fields.
    std::vector<std::shared_ptr<Process>>
    getAllProcesses() const;

    // Returns independent value copies for UI/report readers.
    std::vector<ProcessSnapshot> getSnapshots() const;

    bool processExists(
        const std::string& name
    ) const;

    std::size_t getProcessCount() const;

private:
    // Called while the registry mutex is already locked.
    std::shared_ptr<Process> insert(
        const std::string& name,
        std::vector<Instruction> program
    );

    mutable std::mutex mutex;

    std::vector<std::shared_ptr<Process>> processes;

    int nextPid = 1;

    std::mt19937_64 random;
    InstructionGenerator generator;
};

#endif
