#ifndef SCREEN_MANAGER_H
#define SCREEN_MANAGER_H

#include "ProcessManager.h"

#include <memory>

class ScreenManager {
public:
    // Connects the screen manager to the shared process manager.
    explicit ScreenManager(ProcessManager& manager);

    // Opens a screen for a newly created process.
    bool openScreen(const std::shared_ptr<Process>& process);

    // Reattaches to an existing unfinished process.
    bool reattachScreen(const std::shared_ptr<Process>& process);

    // Displays the currently attached process.
    void displayScreen() const;

    // Detaches from the current process without terminating it.
    void detachScreen();

    // Checks whether a process screen is currently attached.
    bool isAttached() const;

    // Returns the currently displayed process.
    std::shared_ptr<Process> getCurrentProcess() const;

private:
    // Reference to the shared process storage.
    ProcessManager& processManager;

    // Process currently displayed on screen.
    std::shared_ptr<Process> currentProcess = nullptr;

    bool attached = false;
};

#endif
