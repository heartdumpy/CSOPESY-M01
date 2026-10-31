#include "ReportManager.h"

#include <fstream>
#include <iomanip>
#include <ctime>

namespace {
    std::string getStateName(ProcessState state) {
        switch (state) {
            case ProcessState::NEW:
                return "New";
            case ProcessState::READY:
                return "Ready";
            case ProcessState::RUNNING:
                return "Running";
            case ProcessState::WAITING:
                return "Waiting";
            case ProcessState::TERMINATED:
                return "Terminated";
            default:
                return "Unknown";
        }
    }
}

bool ReportManager::generateReport(
    const Scheduler& scheduler,
    const ProcessManager& processManager,
    const std::string& filename,
    std::string& errorMessage
) const {
    std::ofstream file(filename);

    if (!file.is_open()) {
        errorMessage = "Unable to create report file: " + filename;
        return false;
    }

    const auto now = std::chrono::system_clock::to_time_t(
        std::chrono::system_clock::now()
    );

    file << "========== CSOPESY CPU UTILIZATION REPORT ==========\n";
    file << "Generated: " << std::ctime(&now) << "\n";

    file << std::fixed << std::setprecision(2);
    file << "CPU Utilization: "
         << scheduler.getCpuUtilization() << "%\n\n";

    file << "CPU Cores:\n";

    const auto cores = scheduler.getCores();

    for (const auto& core : cores) {
        file << "Core " << core.coreId << ": ";

        if (core.currentProcess) {
            file << core.currentProcess->name;
        }
        else {
            file << "Idle";
        }

        file << " | Active ticks: " << core.activeTicks
             << " | Idle ticks: " << core.idleTicks << "\n";
    }

    file << "\nProcesses:\n";

    const auto processes = processManager.getAllProcesses();

    if (processes.empty()) {
        file << "No processes found.\n";
    }

    for (const auto& process : processes) {
        if (!process) {
            continue;
        }

        file << "PID: " << process->pid
             << " | Name: " << process->name
             << " | State: " << getStateName(process->state)
             << " | Instructions: "
             << process->instructionPointer
             << "/" << process->totalInstructions
             << "\n";
    }

    file << "====================================================\n";

    if (!file.good()) {
        errorMessage = "An error occurred while writing the report.";
        return false;
    }

    errorMessage.clear();
    return true;
}
