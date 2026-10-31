#ifndef COMMAND_INTERPRETER_H
#define COMMAND_INTERPRETER_H

#include "config.h"
#include "ProcessManager.h"
#include "ScreenManager.h"
#include "Scheduler.h"
#include "ReportManager.h"

#include <string>
#include <vector>

class CommandInterpreter {
public:
    // Connects the command interpreter to the shared system components.
    CommandInterpreter(
        ProcessManager& processManager,
        ScreenManager& screenManager,
        Scheduler& scheduler,
        ReportManager& reportManager,
        const Config& config
    );

    // Processes a command entered by the user.
    void executeCommand(const std::string& input);

private:
    ProcessManager& processManager;
    ScreenManager& screenManager;
    Scheduler& scheduler;
    ReportManager& reportManager;
    const Config& config;

    // Separates input into command arguments.
    std::vector<std::string> tokenize(
        const std::string& input
    );

    // Handles commands without subcommands.
    void handleSimpleCommand(const std::string& command);

    // Handles screen-related commands.
    void handleScreenCommand(
        const std::vector<std::string>& arguments
    );
};

#endif
