#include "CommandInterpreter.h"

#include <iostream>
#include <sstream>

CommandInterpreter::CommandInterpreter(
    ProcessManager& manager,
    ScreenManager& screens,
    Scheduler& schedulerInstance,
    ReportManager& reports,
    const Config& configuration
)
    : processManager(manager),
      screenManager(screens),
      scheduler(schedulerInstance),
      reportManager(reports),
      config(configuration) {
}

std::vector<std::string>
CommandInterpreter::tokenize(
    const std::string& input
) {
    std::istringstream stream(input);

    std::vector<std::string> arguments;
    std::string token;

    while (stream >> token) {
        arguments.push_back(token);
    }

    return arguments;
}

void CommandInterpreter::executeCommand(
    const std::string& input
) {
    const std::vector<std::string> arguments =
        tokenize(input);

    if (arguments.empty()) {
        return;
    }

    if (arguments[0] == "screen") {
        handleScreenCommand(arguments);
    }
    else if (arguments.size() != 1) {
        std::cout
            << "Unexpected command arguments.\n";
    }
    else {
        handleSimpleCommand(arguments[0]);
    }
}

void CommandInterpreter::handleSimpleCommand(
    const std::string& command
) {
    if (command == "scheduler-start") {
        if (scheduler.startGeneration()) {
            std::cout
                << "Automatic process generation started.\n";
        }
        else {
            std::cout
                << "Generation already active "
                << "or scheduler unavailable.\n";
        }
    }
    else if (command == "scheduler-stop") {
        scheduler.stopGeneration();

        std::cout
            << "Automatic process generation stopped. "
            << "Existing processes continue.\n";
    }
    else if (command == "process-smi") {
        if (screenManager.isAttached()) {
            screenManager.displayScreen();
        }
        else {
            std::cout
                << "process-smi is available inside "
                << "a process screen.\n";
        }
    }
    else if (command == "report-util") {
        std::string errorMessage;

        const bool saved =
            reportManager.generateReport(
                scheduler,
                processManager,
                "csopesy-log.txt",
                errorMessage
            );

        if (saved) {
            std::cout
                << "Report generated at csopesy-log.txt\n";
        }
        else {
            std::cout
                << "Report generation failed: "
                << errorMessage
                << '\n';
        }
    }
    else {
        std::cout
            << "Unknown command: "
            << command
            << '\n';
    }
}

void CommandInterpreter::handleScreenCommand(
    const std::vector<std::string>& arguments
) {
    if (
        arguments.size() == 2 &&
        arguments[1] == "-ls"
    ) {
        std::cout << reportManager.formatReport(
            scheduler.snapshot()
        );

        return;
    }

    const bool validProcessCommand =
        arguments.size() == 3 &&
        (
            arguments[1] == "-s" ||
            arguments[1] == "-r"
        );

    if (!validProcessCommand) {
        std::cout
            << "Usage: screen -s <name>, "
            << "screen -r <name>, or screen -ls\n";

        return;
    }

    const std::string& processName =
        arguments[2];

    if (arguments[1] == "-s") {
        if (
            processManager.processExists(
                processName
            )
        ) {
            std::cout
                << "A process with that name already exists.\n";

            return;
        }

        try {
            auto process =
                processManager.createProcess(
                    processName,
                    config.minIns,
                    config.maxIns
                );

            if (!process) {
                std::cout
                    << "Process creation failed.\n";

                return;
            }

            screenManager.openScreen(process);
        }
        catch (const std::exception& exception) {
            std::cout
                << "Process creation failed: "
                << exception.what()
                << '\n';
        }
    }
    else {
        auto process =
            processManager.findProcessByName(
                processName
            );

        if (!process) {
            std::cout
                << "Process "
                << processName
                << " not found.\n";
        }
        else {
            screenManager.reattachScreen(process);
        }
    }
}