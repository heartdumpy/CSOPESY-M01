#include "ReportManager.h"

#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace {

std::string formatTimestamp(
    std::chrono::system_clock::time_point point
) {
    const auto rawTime =
        std::chrono::system_clock::to_time_t(point);

    std::tm localTime{};

#ifdef _WIN32
    localtime_s(&localTime, &rawTime);
#else
    localtime_r(&rawTime, &localTime);
#endif

    std::ostringstream output;

    output << std::put_time(
        &localTime,
        "%m/%d/%Y %I:%M:%S%p"
    );

    return output.str();
}

} // namespace

std::string ReportManager::formatReport(
    const SystemSnapshot& system
) const {
    std::ostringstream output;

    output
        << "========== CPU UTILIZATION REPORT ==========\n"
        << std::fixed
        << std::setprecision(2)
        << "CPU utilization: "
        << system.utilization
        << "%\nCores used: "
        << system.coresUsed
        << "\nCores available: "
        << system.coresAvailable
        << "\nCPU tick: "
        << system.tick
        << '\n';

    if (!system.error.empty()) {
        output
            << "Scheduler error: "
            << system.error
            << '\n';
    }

    output
        << "Scheduler: "
        << system.algorithm
        << "\nAutomatic generation: "
        << (system.generating ? "Active" : "Stopped")
        << "\n\nCPU cores:\n";

    for (const CoreSnapshot& core : system.cores) {
        output
            << "Core "
            << core.coreId
            << " | ";

        if (core.currentProcess) {
            output
                << core.currentProcess->name
                << " | Quantum used: "
                << core.quantumUsed
                << " | Delay remaining: "
                << core.delayRemaining;
        }
        else {
            output << "Idle";
        }

        output
            << " | Active ticks: "
            << core.activeTicks
            << " | Idle ticks: "
            << core.idleTicks
            << '\n';
    }

    bool hasUnfinished = false;
    bool hasFinished = false;

    output
        << "\nRunning and unfinished processes:\n";

    for (
        const ProcessSnapshot& process :
        system.processes
    ) {
        if (
            process.state ==
            ProcessState::TERMINATED
        ) {
            continue;
        }

        hasUnfinished = true;

        output
            << process.name
            << " ("
            << formatTimestamp(process.creationTime)
            << ") PID:"
            << process.pid
            << ' ';

        if (
            process.state ==
            ProcessState::RUNNING
        ) {
            output
                << "Core:"
                << process.assignedCore;
        }
        else if (
            process.state ==
            ProcessState::WAITING
        ) {
            output << "Sleeping";
        }
        else {
            output << "Ready";
        }

        output
            << ' '
            << process.instructionPointer
            << " / "
            << process.totalInstructions
            << '\n';
    }

    if (!hasUnfinished) {
        output << "(none)\n";
    }

    output << "\nFinished processes:\n";

    for (
        const ProcessSnapshot& process :
        system.processes
    ) {
        if (
            process.state !=
            ProcessState::TERMINATED
        ) {
            continue;
        }

        hasFinished = true;

        output
            << process.name
            << " ("
            << formatTimestamp(process.creationTime)
            << ") PID:"
            << process.pid
            << " Finished "
            << process.instructionPointer
            << " / "
            << process.totalInstructions
            << '\n';
    }

    if (!hasFinished) {
        output << "(none)\n";
    }

    output
        << "============================================\n";

    return output.str();
}

bool ReportManager::generateReport(
    const Scheduler& scheduler,
    const ProcessManager&,
    const std::string& filename,
    std::string& errorMessage
) const {
    return saveReport(
        scheduler.snapshot(),
        filename,
        errorMessage
    );
}

bool ReportManager::saveReport(
    const SystemSnapshot& system,
    const std::string& filename,
    std::string& errorMessage
) const {
    std::ofstream file(filename);

    if (!file) {
        errorMessage =
            "Unable to create report file: " +
            filename;

        return false;
    }

    file << formatReport(system);

    file.flush();

    if (!file) {
        errorMessage =
            "Error writing report: " +
            filename;

        return false;
    }

    file.close();

    if (file.fail()) {
        errorMessage =
            "Error closing report: " +
            filename;

        return false;
    }

    errorMessage.clear();

    return true;
}