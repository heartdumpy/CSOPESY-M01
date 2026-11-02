#include "ScreenManager.h"

#include <iostream>

ScreenManager::ScreenManager(
    ProcessManager& manager
)
    : processManager(manager) {
}

bool ScreenManager::openScreen(
    const std::shared_ptr<Process>& process
) {
    if (attached || !process) {
        return false;
    }

    if (
        processManager.findProcessByPid(
            process->pid
        ) != process
    ) {
        return false;
    }

    // A tiny new process could finish before its first screen opens.
    // Allow that first screen so its final information can be viewed.
    // Reattachment separately rejects finished processes.
    currentProcess = process;
    attached = true;

    // ANSI clear-screen and cursor-home sequence.
    std::cout << "\x1b[2J\x1b[H";

    displayScreen();

    return true;
}

bool ScreenManager::reattachScreen(
    const std::shared_ptr<Process>& process
) {
    if (attached || !process) {
        return false;
    }

    const ProcessSnapshot information =
        process->snapshot();

    if (
        information.state ==
        ProcessState::TERMINATED
    ) {
        std::cout
            << "Process "
            << information.name
            << " not found.\n";

        return false;
    }

    return openScreen(process);
}

void ScreenManager::displayScreen() const {
    if (!attached || !currentProcess) {
        std::cout
            << "No process screen is currently open.\n";

        return;
    }

    const ProcessSnapshot process =
        currentProcess->snapshot();

    std::cout
        << "\nProcess name: "
        << process.name
        << "\nID: "
        << process.pid
        << "\nLogs:\n";

    for (const std::string& line : process.logs) {
        std::cout << line << '\n';
    }

    std::cout
        << "Current instruction line: "
        << process.instructionPointer
        << "\nLines of code: "
        << process.totalInstructions
        << '\n';

    if (
        process.state ==
        ProcessState::TERMINATED
    ) {
        std::cout << "Finished!\n";
    }

    std::cout
        << "Commands: process-smi, help, exit\n";
}

void ScreenManager::detachScreen() {
    // Does not stop or delete the process.
    currentProcess.reset();
    attached = false;
}

bool ScreenManager::isAttached() const {
    return attached;
}

std::shared_ptr<Process>
ScreenManager::getCurrentProcess() const {
    return currentProcess;
}