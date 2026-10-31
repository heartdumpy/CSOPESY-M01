#include "ScreenManager.h"

#include <iostream>

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

ScreenManager::ScreenManager(ProcessManager& manager)
    : processManager(manager) {
}

bool ScreenManager::openScreen(
    const std::shared_ptr<Process>& process
) {
    if (attached) {
        std::cout << "A process screen is already open.\n";
        return false;
    }

    if (!process) {
        std::cout << "Process does not exist.\n";
        return false;
    }

    // Verify that this process belongs to the ProcessManager.
    std::shared_ptr<Process> registeredProcess =
        processManager.findProcessByPid(process->pid);

    if (!registeredProcess) {
        std::cout << "Process is not registered.\n";
        return false;
    }

    if (registeredProcess->state == ProcessState::TERMINATED) {
        std::cout << "Cannot open a terminated process.\n";
        return false;
    }

    currentProcess = registeredProcess;
    attached = true;

    displayScreen();

    return true;
}

bool ScreenManager::reattachScreen(
    const std::shared_ptr<Process>& process
) {
    if (attached) {
        std::cout << "A process screen is already open.\n";
        return false;
    }

    if (!process) {
        std::cout << "Process does not exist.\n";
        return false;
    }

    std::shared_ptr<Process> registeredProcess =
        processManager.findProcessByPid(process->pid);

    if (!registeredProcess) {
        std::cout << "Process is not registered.\n";
        return false;
    }

    if (registeredProcess->state == ProcessState::TERMINATED) {
        std::cout << "Cannot reattach to a finished process.\n";
        return false;
    }

    currentProcess = registeredProcess;
    attached = true;

    std::cout << "Reattached to process: "
              << currentProcess->name << "\n";

    displayScreen();

    return true;
}

void ScreenManager::displayScreen() const {
    if (!attached || !currentProcess) {
        std::cout << "No process screen is currently open.\n";
        return;
    }

    std::cout << "\n====================================\n";
    std::cout << "             PROCESS SCREEN\n";
    std::cout << "====================================\n";

    std::cout << "Process ID: "
              << currentProcess->pid << "\n";

    std::cout << "Process name: "
              << currentProcess->name << "\n";

    std::cout << "Status: "
              << getStateName(currentProcess->state) << "\n";

    std::cout << "Assigned CPU core: ";

    if (currentProcess->assignedCore == -1) {
        std::cout << "None\n";
    }
    else {
        std::cout << currentProcess->assignedCore << "\n";
    }

    std::cout << "Instructions: "
              << currentProcess->instructionPointer
              << " / "
              << currentProcess->totalInstructions
              << "\n";

    std::cout << "====================================\n";
    std::cout << "Available commands:\n";
    std::cout << "  process-smi\n";
    std::cout << "  help\n";
    std::cout << "  exit\n";
}

void ScreenManager::detachScreen() {
    if (!attached || !currentProcess) {
        std::cout << "No process screen is currently open.\n";
        return;
    }

    std::cout << "Detached from process: "
              << currentProcess->name << "\n";

    // Detaching does not delete the process.
    currentProcess.reset();
    attached = false;
}

bool ScreenManager::isAttached() const {
    return attached;
}

std::shared_ptr<Process> ScreenManager::getCurrentProcess() const {
    return currentProcess;
}
