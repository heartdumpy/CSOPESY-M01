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
    CommandInterpreter(
        ProcessManager& processManager,
        ScreenManager& screenManager,
        Scheduler& scheduler,
        ReportManager& reportManager,
        const Config& config
    );

    void executeCommand(
        const std::string& input
    );

private:
    ProcessManager& processManager;
    ScreenManager& screenManager;

    Scheduler& scheduler;
    ReportManager& reportManager;

    const Config& config;

    std::vector<std::string> tokenize(
        const std::string& input
    );

    void handleSimpleCommand(
        const std::string& command
    );

    void handleScreenCommand(
        const std::vector<std::string>& arguments
    );
};

#endif
