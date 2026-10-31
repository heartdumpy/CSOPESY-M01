
#include "ConsoleManager.h"
#include "CommandInterpreter.h"

#include <iostream>
#include <sstream>

ConsoleManager::ConsoleManager()
    : screenManager(processManager) {
}

ConsoleManager::~ConsoleManager() {
    scheduler.stopScheduler();
}

void ConsoleManager::run() {
    std::cout << "====================================\n";
    std::cout << "          CSOPESY OS EMULATOR\n";
    std::cout << "====================================\n";

    displayMainMenu();

    std::string input;

    while (running) {
        std::cout << "\nCSOPESY> ";

        if (!std::getline(std::cin, input)) {
            running = false;
            break;
        }

        processInput(input);
    }

    scheduler.stopScheduler();

    std::cout << "\nCSOPESY emulator shutting down...\n";
}

void ConsoleManager::displayMainMenu() const {
    std::cout << "\nAvailable commands:\n";
    std::cout << "  initialize\n";
    std::cout << "  screen -s <process name>\n";
    std::cout << "  screen -r <process name>\n";
    std::cout << "  screen -ls\n";
    std::cout << "  scheduler-start\n";
    std::cout << "  scheduler-stop\n";
    std::cout << "  process-smi\n";
    std::cout << "  report-util\n";
    std::cout << "  exit\n";
}

void ConsoleManager::processInput(const std::string& input) {
    std::istringstream stream(input);
    std::string command;

    stream >> command;

    if (command.empty()) {
        return;
    }

    // Exit is always available, even before initialization.
    if (command == "exit") {
        running = false;
        scheduler.stopScheduler();
        return;
    }

    // Initialization must happen before other commands.
    if (!initialized && command != "initialize") {
        std::cout << "System is not initialized.\n";
        std::cout << "Please enter 'initialize' first.\n";
        return;
    }

    if (command == "initialize") {
        if (initialized) {
            std::cout << "System is already initialized.\n";
        }
        else {
            initializeSystem();
        }

        return;
    }

    CommandInterpreter interpreter(
        processManager,
        screenManager,
        scheduler,
        reportManager,
        config
    );

    interpreter.executeCommand(input);

    // Enter the process screen if a screen was opened.
    if (screenManager.isAttached()) {
        runProcessScreen();
    }
}

bool ConsoleManager::initializeSystem() {
    std::string errorMessage;

    if (!loadConfig("config.txt", config, errorMessage)) {
        std::cout << "\nInitialization failed!\n";
        std::cout << errorMessage << "\n";
        std::cout << "Please check your config.txt file.\n";

        initialized = false;
        return false;
    }

    initialized = true;

    std::cout << "\nSystem initialized successfully!\n";
    std::cout << "Number of CPU cores: "
              << config.numCpu << "\n";
    std::cout << "Scheduler: "
              << config.scheduler << "\n";
    std::cout << "Quantum cycles: "
              << config.quantumCycles << "\n";

    return true;
}

void ConsoleManager::runProcessScreen() {
    CommandInterpreter interpreter(
        processManager,
        screenManager,
        scheduler,
        reportManager,
        config
    );

    while (screenManager.isAttached() && running) {
        std::cout << "\nPROCESS> ";

        std::string input;

        if (!std::getline(std::cin, input)) {
            screenManager.detachScreen();
            running = false;
            break;
        }

        if (input == "exit") {
            screenManager.detachScreen();
            continue;
        }

        if (input == "process-smi") {
            interpreter.executeCommand(input);
            continue;
        }

        if (input == "help") {
            std::cout << "\nProcess screen commands:\n";
            std::cout << "  process-smi\n";
            std::cout << "  help\n";
            std::cout << "  exit\n";
            continue;
        }

        if (input.empty()) {
            continue;
        }

        std::cout << "Unknown process screen command: "
                  << input << "\n";
        std::cout << "Type 'help' to see available commands.\n";
    }
}
