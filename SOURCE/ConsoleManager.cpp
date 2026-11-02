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
    std::cout
        << "====================================\n"
        << "          CSOPESY OS EMULATOR\n"
        << "====================================\n";

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

    std::cout
        << "\nCSOPESY emulator shutting down...\n";
}

void ConsoleManager::displayMainMenu() const {
    std::cout
        << "\nAvailable commands:\n"
        << "  initialize\n"
        << "  screen -s <process name>\n"
        << "  screen -r <process name>\n"
        << "  screen -ls\n"
        << "  scheduler-start\n"
        << "  scheduler-stop\n"
        << "  report-util\n"
        << "  exit\n";
}

void ConsoleManager::processInput(
    const std::string& input
) {
    std::istringstream stream(input);

    std::string command;
    stream >> command;

    std::string extra;

    if (
        (
            command == "exit" ||
            command == "initialize"
        ) &&
        (stream >> extra)
    ) {
        std::cout
            << "Unexpected command arguments.\n";

        return;
    }

    if (command.empty()) {
        return;
    }

    // Exit is available before initialization.
    if (command == "exit") {
        running = false;
        scheduler.stopScheduler();

        return;
    }

    if (
        !initialized &&
        command != "initialize"
    ) {
        std::cout
            << "System is not initialized.\n"
            << "Please enter 'initialize' first.\n";

        return;
    }

    if (command == "initialize") {
        if (initialized) {
            std::cout
                << "System is already initialized.\n";
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

    if (screenManager.isAttached()) {
        runProcessScreen();
    }
}

bool ConsoleManager::initializeSystem() {
    std::string errorMessage;

    if (
        !loadConfig(
            "config.txt",
            config,
            errorMessage
        )
    ) {
        std::cout
            << "\nInitialization failed!\n"
            << errorMessage
            << "\nPlease check your config.txt file.\n";

        initialized = false;

        return false;
    }

    // Start execution immediately after successful initialization.
    // Automatic process generation remains off until scheduler-start.
    if (
        !scheduler.initialize(
            processManager,
            config
        )
    ) {
        std::cout
            << "Unable to initialize scheduler.\n";

        return false;
    }

    initialized = true;

    std::cout
        << "\nSystem initialized successfully!\n"
        << "Number of CPU cores: "
        << config.numCpu
        << "\nScheduler: "
        << config.scheduler
        << "\nQuantum cycles: "
        << config.quantumCycles
        << '\n';

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

    while (
        screenManager.isAttached() &&
        running
    ) {
        std::cout << "\nPROCESS> ";

        std::string input;

        if (!std::getline(std::cin, input)) {
            screenManager.detachScreen();
            running = false;

            break;
        }

        std::istringstream commandStream(input);

        std::string screenCommand;
        std::string extra;

        commandStream >> screenCommand;

        if (commandStream >> extra) {
            std::cout
                << "Unexpected process command arguments.\n";

            continue;
        }

        input = screenCommand;

        if (input == "exit") {
            screenManager.detachScreen();
            continue;
        }

        if (input == "process-smi") {
            interpreter.executeCommand(input);
            continue;
        }

        if (input == "help") {
            std::cout
                << "\nProcess screen commands:\n"
                << "  process-smi\n"
                << "  help\n"
                << "  exit\n";

            continue;
        }

        if (input.empty()) {
            continue;
        }

        std::cout
            << "Unknown process screen command: "
            << input
            << "\nType 'help' to see available commands.\n";
    }
}