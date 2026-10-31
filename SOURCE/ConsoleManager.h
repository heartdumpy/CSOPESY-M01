
#ifndef CONSOLE_MANAGER_H
#define CONSOLE_MANAGER_H

#include "config.h"
#include "ProcessManager.h"
#include "ScreenManager.h"
#include "Scheduler.h"
#include "ReportManager.h"

#include <string>

class ConsoleManager {
public:
    ConsoleManager();
    ~ConsoleManager();

    void run();

private:
    bool initialized = false;
    bool running = true;

    Config config{};

    // Shared system components.
    ProcessManager processManager;
    ScreenManager screenManager;
    Scheduler scheduler;
    ReportManager reportManager;

    void displayMainMenu() const;
    void processInput(const std::string& input);
    bool initializeSystem();
    void runProcessScreen();
};

#endif
