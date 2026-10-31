
#include "CommandInterpreter.h"

#include <iostream>
#include <iomanip>
#include <sstream>

namespace {
    std::string getStateName(ProcessState state) {
        switch (state) {
            case ProcessState::NEW: return "New";
            case ProcessState::READY: return "Ready";
            case ProcessState::RUNNING: return "Running";
            case ProcessState::WAITING: return "Waiting";
            case ProcessState::TERMINATED: return "Terminated";
            default: return "Unknown";
        }
    }
}

CommandInterpreter::CommandInterpreter(
    ProcessManager& processManager,
    ScreenManager& screenManager,
    Scheduler& scheduler,
    ReportManager& reportManager,
    const Config& config
)
    : processManager(processManager),
      screenManager(screenManager),
      scheduler(scheduler),
      reportManager(reportManager),
      config(config) {
}

std::vector<std::string> CommandInterpreter::tokenize(
    const std::string& input
) {
    std::vector<std::string> arguments;
    std::istringstream stream(input);
    std::string token;

    while (stream >> token) {
        arguments.push_back(token);
    }

    return arguments;
}

void CommandInterpreter::executeCommand(const std::string& input) {
    std::vector<std::string> arguments = tokenize(input);

    if (arguments.empty()) {
        return;
    }

    if (arguments[0] == "screen") {
        handleScreenCommand(arguments);
    }
    else {
        handleSimpleCommand(arguments[0]);
    }
}

void CommandInterpreter::handleSimpleCommand(
    const std::string& command
) {
    if (command == "scheduler-start") {
        if (scheduler.isRunning()) {
            std::cout << "Scheduler is already running.\n";
            return;
        }

        if (scheduler.startScheduler(processManager, config)) {
            std::cout << "Scheduler started successfully.\n";
        }
        else {
            std::cout << "Unable to start scheduler.\n";
        }
    }
    else if (command == "scheduler-stop") {
        if (!scheduler.isRunning()) {
            std::cout << "Scheduler is not running.\n";
            return;
        }

        scheduler.stopScheduler();
        std::cout << "Scheduler stopped.\n";
    }
    else if (command == "process-smi") {
        std::cout << "\n========== PROCESS SMI ==========\n";

        std::cout << "CPU Utilization: "
                  << std::fixed << std::setprecision(2)
                  << scheduler.getCpuUtilization() << "%\n";

        std::cout << "\nCPU Cores:\n";

        for (const CPUCore& core : scheduler.getCores()) {
            std::cout << "Core " << core.coreId << ": ";

            if (core.currentProcess) {
                std::cout << core.currentProcess->name;
            }
            else {
                std::cout << "Idle";
            }

            std::cout << "\n";
        }

        std::cout << "\nProcesses:\n";

        for (const auto& process : processManager.getAllProcesses()) {
            if (!process) {
                continue;
            }

            std::cout << "PID: " << process->pid
                      << " | Name: " << process->name
                      << " | State: " << getStateName(process->state)
                      << " | Instructions: "
                      << process->instructionPointer
                      << "/" << process->totalInstructions
                      << "\n";
        }

        std::cout << "=================================\n";
    }
    else if (command == "report-util") {
        std::string errorMessage;

        if (reportManager.generateReport(
                scheduler,
                processManager,
                "csopesy-log.txt",
                errorMessage)) {
            std::cout << "CPU utilization report generated: "
                      << "csopesy-log.txt\n";
        }
        else {
            std::cout << "Report generation failed: "
                      << errorMessage << "\n";
        }
    }
    else {
        std::cout << "Unknown command: " << command << "\n";
    }
}

void CommandInterpreter::handleScreenCommand(
    const std::vector<std::string>& arguments
) {
    if (arguments.size() < 2) {
        std::cout << "Invalid screen command.\n";
        return;
    }

    if (arguments[1] == "-s") {
        if (arguments.size() != 3) {
            std::cout << "Usage: screen -s <process name>\n";
            return;
        }

        if (processManager.processExists(arguments[2])) {
            std::cout << "A process with that name already exists.\n";
            return;
        }

        std::shared_ptr<Process> process =
            processManager.createProcess(
                arguments[2],
                config.minIns,
                config.maxIns
            );

        if (!process) {
            std::cout << "Process creation failed.\n";
            return;
        }

        std::cout << "Process created successfully: "
                  << process->name << "\n";

        screenManager.openScreen(process);
    }
    else if (arguments[1] == "-r") {
        if (arguments.size() != 3) {
            std::cout << "Usage: screen -r <process name>\n";
            return;
        }

        std::shared_ptr<Process> process =
            processManager.findProcessByName(arguments[2]);

        if (!process) {
            std::cout << "Process not found: "
                      << arguments[2] << "\n";
            return;
        }

        screenManager.reattachScreen(process);
    }
    else if (arguments[1] == "-ls") {
        std::vector<std::shared_ptr<Process>> processes =
            processManager.getAllProcesses();

        std::cout << "\n========== PROCESS LIST ==========\n";

        if (processes.empty()) {
            std::cout << "No processes found.\n";
        }

        for (const auto& process : processes) {
            if (!process) {
                continue;
            }

            std::cout << "PID: " << process->pid
                      << " | Name: " << process->name
                      << " | State: " << getStateName(process->state)
                      << " | Instructions: "
                      << process->instructionPointer
                      << "/" << process->totalInstructions
                      << "\n";
        }

        std::cout << "==================================\n";
    }
    else {
        std::cout << "Unknown screen option: "
                  << arguments[1] << "\n";
    }
}
